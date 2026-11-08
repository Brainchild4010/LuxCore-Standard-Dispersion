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

#if !defined(LUXRAYS_DISABLE_OPENCL)

#include "slg/engines/pathoclbase/compiledscene.h"

using namespace std;
using namespace luxrays;
using namespace slg;

void CompiledScene::CompilePathTracer() {
	// Keep the wavelength in a low Sobol dimension directly after the camera
	// boot samples. High dimensions (hundreds for deep paths) produced visible
	// device/seed dependent color casts at practical sample counts.
	compiledPathTracer.eyeSampleBootSize = pathTracer->eyeSampleBootSize +
			(pathTracer->IsStandardDispersionSampleDimensionReserved() ? 0 : 1);
	compiledPathTracer.eyeSampleStepSize = pathTracer->eyeSampleStepSize;
	// Reserve one additional sampler dimension for GPU-only engines. PATHOCL
	// already reserves the same low dimension in the shared PathTracer layout so
	// its native CPU and GPU threads use an identical wavelength dimension.
	compiledPathTracer.eyeSampleSize = pathTracer->eyeSampleSize +
			(pathTracer->IsStandardDispersionSampleDimensionReserved() ? 0 : 1);

	compiledPathTracer.maxPathDepth.depth = pathTracer->maxPathDepth.depth;
	compiledPathTracer.maxPathDepth.diffuseDepth = pathTracer->maxPathDepth.diffuseDepth;
	compiledPathTracer.maxPathDepth.glossyDepth = pathTracer->maxPathDepth.glossyDepth;
	compiledPathTracer.maxPathDepth.specularDepth = pathTracer->maxPathDepth.specularDepth;
	
	compiledPathTracer.rrDepth = pathTracer->rrDepth;
	compiledPathTracer.rrImportanceCap = pathTracer->rrImportanceCap;
	
	compiledPathTracer.sqrtVarianceClampMaxValue = pathTracer->sqrtVarianceClampMaxValue;

	compiledPathTracer.hybridBackForward.enabled = pathTracer->hybridBackForwardEnable;
	compiledPathTracer.hybridBackForward.glossinessThreshold = pathTracer->hybridBackForwardGlossinessThreshold;
	
	compiledPathTracer.forceBlackBackground = pathTracer->forceBlackBackground;

	CompilePhotonGI();

	compiledPathTracer.albedo.specularSetting = (slg::ocl::AlbedoSpecularSetting)pathTracer->albedoSpecularSetting;
	compiledPathTracer.albedo.specularGlossinessThreshold = pathTracer->albedoSpecularGlossinessThreshold;
}

#endif
// vim: autoindent noexpandtab tabstop=4 shiftwidth=4
