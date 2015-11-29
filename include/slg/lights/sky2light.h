/***************************************************************************
 * Copyright 1998-2015 by authors (see AUTHORS.txt)                        *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
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

#ifndef _SLG_SKY2LIGHT_H
#define	_SLG_SKY2LIGHT_H

#include "slg/lights/light.h"

namespace slg {

//------------------------------------------------------------------------------
// Sky2 implementation
//------------------------------------------------------------------------------

class SkyLight2 : public EnvLightSource {
public:
	SkyLight2();
	virtual ~SkyLight2();

	virtual void Preprocess();
	void GetPreprocessedData(float *absoluteDirData,
		float *aTermData, float *bTermData, float *cTermData, float *dTermData,
		float *eTermData, float *fTermData, float *gTermData, float *hTermData,
		float *iTermData,float *radianceTermData) const;

	virtual LightSourceType GetType() const { return TYPE_IL_SKY2; }
	virtual float GetPower(const Scene &scene) const;

	virtual luxrays::Spectrum Emit(const Scene &scene,
		const float u0, const float u1, const float u2, const float u3, const float passThroughEvent,
		luxrays::Point *pos, luxrays::Vector *dir,
		float *emissionPdfW, float *directPdfA = NULL, float *cosThetaAtLight = NULL) const;

    virtual luxrays::Spectrum Illuminate(const Scene &scene, const luxrays::Point &p,
		const float u0, const float u1, const float passThroughEvent,
        luxrays::Vector *dir, float *distance, float *directPdfW,
		float *emissionPdfW = NULL, float *cosThetaAtLight = NULL) const;

	virtual luxrays::Spectrum GetRadiance(const Scene &scene, const luxrays::Vector &dir,
			float *directPdfA = NULL, float *emissionPdfW = NULL) const;

	virtual luxrays::Properties ToProperties(const ImageMapCache &imgMapCache) const;

	luxrays::Vector localSunDir;
	float turbidity;
	luxrays::Spectrum groundAlbedo;

private:
	luxrays::Spectrum ComputeRadiance(const luxrays::Vector &w) const;
	float ComputeY(const luxrays::Vector &w) const;

	luxrays::Vector absoluteSunDir;
	luxrays::Spectrum model[10];
	luxrays::Spectrum aTerm, bTerm, cTerm, dTerm, eTerm, fTerm,
		gTerm, hTerm, iTerm, radianceTerm;
};

}

#endif	/* _SLG_SKY2LIGHT_H */
