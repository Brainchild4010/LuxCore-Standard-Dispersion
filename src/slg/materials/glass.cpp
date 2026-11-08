/***************************************************************************
 * Copyright 1998-2020 by authors (see AUTHORS.txt)                        *
 *                                                                         *
 *   This file is part of LuxCoreRender.                                   *
 *                                                                         *
 * Licensed under the Apache License, Version 2.0 (the "License");         *
 * you may not use this file except in compliance with the License.        *
 * You may obtain a copy of the License at                                 *
 *                                                                         *
 *     http://www.apache.org/licenses/LICENSE-2.0                          *
 *                                                                         *
 * Unless required by applicable law or agreed to in writing, software     *
 * distributed under the License is distributed on an "AS IS" BASIS,       *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
 * See the License for the specific language governing permissions and     *
 * limitations under the License.                                          *
 ***************************************************************************/

#include "slg/textures/fresnel/fresneltexture.h"
#include "slg/materials/glass.h"
#include "slg/materials/thinfilmcoating.h"

using namespace std;
using namespace luxrays;
using namespace slg;

// A complete integrated CPU path sample uses one wavelength. The state is
// thread-local because each render thread traces one sample at a time. The
// bounce flag reports whether the just sampled BSDF event was wavelength-
// dependent.
static thread_local float standardDispersionWaveLength = -1.f;
static thread_local bool standardDispersionBounceEvent = false;

static bool IsStandardDispersionPathSampleActive() {
	return (standardDispersionWaveLength >= 380.f) &&
			(standardDispersionWaveLength <= 780.f);
}

void slg::SetStandardDispersionPathSample(const float waveLength) {
	standardDispersionWaveLength = Clamp(waveLength, 380.f, 780.f);
	standardDispersionBounceEvent = false;
}

void slg::ClearStandardDispersionPathSample() {
	standardDispersionWaveLength = -1.f;
	standardDispersionBounceEvent = false;
}

void slg::ResetStandardDispersionBounceEvent() {
	standardDispersionBounceEvent = false;
}

bool slg::ConsumeStandardDispersionBounceEvent() {
	const bool result = standardDispersionBounceEvent;
	standardDispersionBounceEvent = false;
	return result;
}

static float GetStandardDispersionWaveLength(const float fallbackU) {
	if (IsStandardDispersionPathSampleActive())
		return standardDispersionWaveLength;

	return Lerp(fallbackU, 380.f, 780.f);
}

//------------------------------------------------------------------------------
// Glass material
//------------------------------------------------------------------------------

GlassMaterial::GlassMaterial(TextureConstPtr frontTransp, TextureConstPtr backTransp,
		TextureConstPtr emitted, TextureConstPtr bump,
		TextureConstPtr refl, TextureConstPtr trans,
		TextureConstPtr exteriorIorFact, TextureConstPtr interiorIorFact,
		TextureConstPtr B, TextureConstPtr filmThickness, TextureConstPtr filmIor) :
			Material(frontTransp, backTransp, emitted, bump),
			Kr(refl), Kt(trans), exteriorIor(exteriorIorFact), interiorIor(interiorIorFact),
			cauchyB(B), filmThickness(filmThickness), filmIor(filmIor) {
}

Spectrum GlassMaterial::Evaluate(const HitPoint &hitPoint,
	const Vector &localLightDir, const Vector &localEyeDir, BSDFEvent *event,
	float *directPdfW, float *reversePdfW) const {
	return Spectrum();
}

static Spectrum WaveLength2RGB(const float waveLength) {
	float r, g, b;
	if ((waveLength >= 380.f) && (waveLength < 440.f)) {
		r = -(waveLength - 440.f) / (440 - 380.f);
		g = 0.f;
		b = 1.f;
	} else if ((waveLength >= 440.f) && (waveLength < 490.f)) {
		r = 0.f;
		g = (waveLength - 440.f) / (490.f - 440.f);
		b = 1.f;
	} else if ((waveLength >= 490.f) && (waveLength < 510.f)) {
		r = 0.f;
		g = 1.f;
		b = -(waveLength - 510.f) / (510.f - 490.f);
	} else if ((waveLength >= 510.f) && (waveLength < 580.f)) {
		r = (waveLength - 510.f) / (580.f - 510.f);
		g = 1.f;
		b = 0.f;
	} else if ((waveLength >= 580.f) && (waveLength < 645.f)) {
		r = 1.f;
		g = -(waveLength - 645.f) / (645 - 580.f);
		b = 0.f;
	} else if ((waveLength >= 645.f) && (waveLength < 780.f)) {
		r = 1.f;
		g = 0.f;
		b = 0.f;
	} else
		return Spectrum();

	// The intensity fall off near the upper and lower limits
	float factor;
	if ((waveLength >= 380.f) && (waveLength < 420.f))
		factor = .3f + .7f * (waveLength - 380.f) / (420.f - 380.f);
	else if ((waveLength >= 420) && (waveLength < 700))
		factor = 1.f;
	else
		factor = .3f + .7f * (780.f - waveLength) / (780.f - 700.f);

	const Spectrum result = Spectrum(r, g, b) * factor;

	/*
	Spectrum white;
	for (u_int i = 380; i < 780; ++i)
		white += WaveLength2RGB(i);
	white *= 1.f / 400.f;
	cout << std::setprecision(std::numeric_limits<float>::digits10 + 1) << white.c[0] << ", " << white.c[1] << ", " << white.c[2] << "\n";
	 
	 Result: 0.5652729, 0.36875, 0.265375
	 */

	// To normalize the output
	const Spectrum normFactor(1.f / .5652729f, 1.f / .36875f, 1.f / .265375f);
	
	return result * normFactor;
}

// Multi-lobe analytic fit of the CIE 1931 2-degree standard observer from:
// Wyman, Sloan and Shirley, "Simple Analytic Approximations to the CIE XYZ
// Color Matching Functions", JCGT 2(2), 2013.
static float CIE1931GaussianFit(const float waveLength, const float alpha,
		const float beta, const float gamma, const float delta) {
	const float t = (waveLength - beta) *
			((waveLength < beta) ? gamma : delta);
	return alpha * expf(-.5f * t * t);
}

static Spectrum WaveLength2CIERGB(const float waveLength) {
	if ((waveLength < 380.f) || (waveLength > 780.f))
		return Spectrum();

	const float x =
			CIE1931GaussianFit(waveLength, .362f, 442.f, .0624f, .0374f) +
			CIE1931GaussianFit(waveLength, 1.056f, 599.8f, .0264f, .0323f) +
			CIE1931GaussianFit(waveLength, -.065f, 501.1f, .049f, .0382f);
	const float y =
			CIE1931GaussianFit(waveLength, .821f, 568.8f, .0213f, .0247f) +
			CIE1931GaussianFit(waveLength, .286f, 530.9f, .0613f, .0322f);
	const float z =
			CIE1931GaussianFit(waveLength, 1.217f, 437.f, .0845f, .0278f) +
			CIE1931GaussianFit(waveLength, .681f, 459.f, .0385f, .0725f);

	// A monochromatic CIE color can be outside the renderer's RGB gamut.
	// Clip only negative display primaries; the per-channel normalization below
	// preserves a neutral unit response for a uniform 380-780 nm spectrum.
	// Keep the CPU and OpenCL/CUDA implementations bit-for-bit aligned. This is
	// the same LuxCore XYZ-to-RGB matrix used by the OpenCL thin-film code and
	// GlassMaterial_WaveLength2CIERGB().
	const RGBColor rgb(
			2.868713378906250f * x - 1.423546671867371f * y - .445165514945984f * z,
			-1.132583618164062f * x + 2.095324993133545f * y + .037260532379150f * z,
			.061925172805786f * x - .216627478599548f * y + 1.154703855514526f * z);
	const Spectrum positiveRGB(Max(0.f, rgb.c[0]), Max(0.f, rgb.c[1]),
			Max(0.f, rgb.c[2]));

	// Reciprocal mean of positiveRGB over a uniform continuous 380-780 nm
	// wavelength distribution, evaluated with the shared CPU/GPU matrix.
	const Spectrum normFactor(2.6351948f, 3.1797323f, 3.3476266f);
	return positiveRGB * normFactor;
}

Spectrum slg::GetStandardDispersionSampleColor() {
	if (!IsStandardDispersionPathSampleActive())
		return Spectrum(1.f);

	return WaveLength2CIERGB(standardDispersionWaveLength);
}

static float WaveLength2IOR(const float waveLength, const float IOR, const float B) {
	// Cauchy's equation for relationship between the refractive index and wavelength
	// note: Cauchy's lambda is expressed in micrometers while waveLength is in nanometers

	// This is the formula suggested by Neo here, with a changed naming convention from B->A and C-> B:
	// https://github.com/LuxCoreRender/BlendLuxCore/commit/d3fed046ab62e18226e410b42a16ca1bccefb530#commitcomment-26617643
	
	// Compute Cauchy-A assuming the user input IOR at 587.56 nm
	// (Fraunhofer d-line, Helium, used in one definition of the Abbe number)
	const float A = IOR - B / Sqr(587.56f / 1000.f);

	// Cauchy's equation
	const float cauchyEq = A + B / Sqr(waveLength / 1000.f);

	return cauchyEq;
}

Spectrum GlassMaterial::EvalSpecularReflection(const HitPoint &hitPoint,
		const Vector &localFixedDir, const float u0, const Spectrum &kr,
		const float nc, const float nt, const float cauchyB,
		Vector *localSampledDir, 
		const float localFilmThickness, const float localFilmIor) {
	if (kr.Black())
		return Spectrum();

	const float cosTheta = CosTheta(localFixedDir);
	*localSampledDir = Vector(-localFixedDir.x, -localFixedDir.y, localFixedDir.z);

	const float lnt = ((cauchyB > 0.f) &&
			IsStandardDispersionPathSampleActive()) ?
			WaveLength2IOR(GetStandardDispersionWaveLength(u0), nt, cauchyB) : nt;
	const float ntc = lnt / nc;
	const Spectrum result = kr * FresnelTexture::CauchyEvaluate(ntc, cosTheta);

	if (localFilmThickness > 0.f) {
		const Spectrum filmColor = CalcFilmColor(localFixedDir, localFilmThickness, localFilmIor);
		return result * filmColor;
	}
	return result;
}

Spectrum GlassMaterial::EvalSpecularTransmission(const HitPoint &hitPoint,
		const Vector &localFixedDir, const float u0,
		const Spectrum &kt, const float nc, const float nt, const float cauchyB,
		Vector *localSampledDir) {
	if (kt.Black())
		return Spectrum();

	// Compute transmitted ray direction
	Spectrum lkt;
	float lnt;
	if (cauchyB > 0.f) {
		// Reuse the wavelength selected once for an enabled integrated CPU path.
		// Other engines retain LuxCore's original per-bounce behavior until
		// they receive their own complete path-level integration.
		const float waveLength = GetStandardDispersionWaveLength(u0);
		lnt = WaveLength2IOR(waveLength, nt, cauchyB);

		if (IsStandardDispersionPathSampleActive())
			// RGB reconstruction is applied once to each completed contribution.
			lkt = kt;
		else
			// Preserve the original LuxCore estimator in engines not yet integrated.
			lkt = kt * WaveLength2RGB(waveLength);
	} else {
		lnt = nt;
		lkt = kt;
	}

	const float ntc = lnt / nc;
	const float cosTheta = CosTheta(localFixedDir);
	const bool entering = (cosTheta > 0.f);
	const float eta = entering ? (nc / lnt) : ntc;
	const float eta2 = eta * eta;
	const float sini2 = SinTheta2(localFixedDir);
	const float sint2 = eta2 * sini2;

	// Handle total internal reflection for transmission
	if (sint2 >= 1.f)
		return Spectrum();
	const float cost = sqrtf(Max(0.f, 1.f - sint2)) * (entering ? -1.f : 1.f);
	*localSampledDir = Vector(-eta * localFixedDir.x, -eta * localFixedDir.y, cost);

	float ce;
	if (!hitPoint.fromLight)
		ce = (1.f - FresnelTexture::CauchyEvaluate(ntc, cost)) * eta2;
	else {
		const float absCosSampledDir = fabsf(CosTheta(*localSampledDir));
		ce = (1.f - FresnelTexture::CauchyEvaluate(ntc, cosTheta)) * fabsf(CosTheta(localFixedDir) / absCosSampledDir);
	}

	return lkt * ce;
}

Spectrum GlassMaterial::Sample(const HitPoint &hitPoint,
		const Vector &localFixedDir, Vector *localSampledDir,
		const float u0, const float u1, const float passThroughEvent,
		float *pdfW, BSDFEvent *event) const {
	const Spectrum kr = Kr->GetSpectrumValue(hitPoint).Clamp(0.f, 1.f);
	const Spectrum kt = Kt->GetSpectrumValue(hitPoint).Clamp(0.f, 1.f);

	const float nc = ExtractExteriorIors(hitPoint, exteriorIor);
	const float nt = ExtractInteriorIors(hitPoint, interiorIor);

	const float cauchyBValue = cauchyB ? cauchyB->GetFloatValue(hitPoint) : 0.f;

	Vector transLocalSampledDir; 
	const Spectrum trans = EvalSpecularTransmission(hitPoint, localFixedDir, u0,
			kt, nc, nt, cauchyBValue, &transLocalSampledDir);
	
	const float localFilmThickness = filmThickness ? filmThickness->GetFloatValue(hitPoint) : 0.f;
	const float localFilmIor = (localFilmThickness > 0.f && filmIor) ? filmIor->GetFloatValue(hitPoint) : 1.f;
	Vector reflLocalSampledDir;
	const Spectrum refl = EvalSpecularReflection(hitPoint, localFixedDir,
			u0, kr, nc, nt, cauchyBValue, &reflLocalSampledDir,
			localFilmThickness, localFilmIor);

	// Decide to transmit or reflect
	float threshold;
	if (!refl.Black()) {
		if (!trans.Black()) {
			// Importance sampling
			const float reflFilter = refl.Filter();
			const float transFilter = trans.Filter();
			threshold = transFilter / (reflFilter + transFilter);

			// A place an upper and lower limit to not under sample
			// reflection or transmission
			threshold = Clamp(threshold, .25f, .75f);
		} else
			threshold = 0.f;
	} else {
		if (!trans.Black())
			threshold = 1.f;
		else
			return Spectrum();
	}

	Spectrum result;
	if (passThroughEvent < threshold) {
		// Transmit

		*localSampledDir = transLocalSampledDir;

		*event = SPECULAR | TRANSMIT;
		*pdfW = threshold;
	
		result = trans;
	} else {
		// Reflect

		*localSampledDir = reflLocalSampledDir;

		*event = SPECULAR | REFLECT;
		*pdfW = 1.f - threshold;
		
		result = refl;
	}

	if ((cauchyBValue > 0.f) && IsStandardDispersionPathSampleActive())
		standardDispersionBounceEvent = true;
	
	return result / *pdfW;
}

void GlassMaterial::Pdf(const HitPoint &hitPoint,
		const luxrays::Vector &localLightDir, const luxrays::Vector &localEyeDir,
	float *directPdfW, float *reversePdfW) const {
	if (directPdfW)
		*directPdfW = 0.f;
	if (reversePdfW)
		*reversePdfW = 0.f;
}

void GlassMaterial::AddReferencedTextures(std::unordered_set<const Texture *>  &referencedTexs) const {
	Material::AddReferencedTextures(referencedTexs);

	Kr->AddReferencedTextures(referencedTexs);
	Kt->AddReferencedTextures(referencedTexs);
	if (exteriorIor)
		exteriorIor->AddReferencedTextures(referencedTexs);
	if (interiorIor)
		interiorIor->AddReferencedTextures(referencedTexs);
	if (filmThickness)
		filmThickness->AddReferencedTextures(referencedTexs);
	if (filmIor)
		filmIor->AddReferencedTextures(referencedTexs);
}

void GlassMaterial::UpdateTextureReferences(TextureConstRef oldTex, TextureRef newTex) {
	Material::UpdateTextureReferences(oldTex, newTex);

	if (Kr == &oldTex)
		Kr = &newTex;
	if (Kt == &oldTex)
		Kt = &newTex;
	if (exteriorIor == &oldTex)
		exteriorIor = &newTex;
	if (interiorIor == &oldTex)
		interiorIor = &newTex;
	if (filmThickness == &oldTex)
		filmThickness = &newTex;
	if (filmIor == &oldTex)
		filmIor = &newTex;
}

PropertiesUPtr GlassMaterial::ToProperties(const ImageMapCache &imgMapCache, const bool useRealFileName) const  {
	auto props = std::make_unique<Properties>();

	const string name = GetName();
	props->Set(Property("scene.materials." + name + ".type")("glass"));
	props->Set(Property("scene.materials." + name + ".kr")(Kr->GetSDLValue()));
	props->Set(Property("scene.materials." + name + ".kt")(Kt->GetSDLValue()));
	if (exteriorIor)
		props->Set(Property("scene.materials." + name + ".exteriorior")(exteriorIor->GetSDLValue()));
	if (interiorIor)
		props->Set(Property("scene.materials." + name + ".interiorior")(interiorIor->GetSDLValue()));
	if (cauchyB)
		props->Set(Property("scene.materials." + name + ".cauchyb")(cauchyB->GetSDLValue()));
	if (filmThickness)
		props->Set(Property("scene.materials." + name + ".filmthickness")(filmThickness->GetSDLValue()));
	if (filmIor)
		props->Set(Property("scene.materials." + name + ".filmior")(filmIor->GetSDLValue()));
	props->Set(Material::ToProperties(imgMapCache, useRealFileName));

	return props;
}
// vim: autoindent noexpandtab tabstop=4 shiftwidth=4
