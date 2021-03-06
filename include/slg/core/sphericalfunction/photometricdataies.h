/***************************************************************************
 * Copyright 1998-2018 by authors (see AUTHORS.txt)                        *
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

#ifndef _SLG_PHOTOMETRICDATAIES_H
#define _SLG_PHOTOMETRICDATAIES_H

#include <istream>
#include <sstream>
#include <cstring>

#include <boost/unordered_map.hpp>

namespace slg {

class PhotometricDataIES {
public:
	PhotometricDataIES();
	PhotometricDataIES(const char *);
	PhotometricDataIES(std::istringstream &is);

	~PhotometricDataIES();

	//--------------------------------------------------------------------------
	// Methods
	//--------------------------------------------------------------------------
	
	bool IsValid() { return m_bValid; }
	void Reset();
	bool Load(const char *sFileName);
	bool Load(std::istringstream &is);
	
	//--------------------------------------------------------------------------
	// Keywords and light descriptions
	//--------------------------------------------------------------------------

	std::string m_Version;
	boost::unordered_map<std::string, std::string> m_Keywords;

	//--------------------------------------------------------------------------
	// Light data.
	//--------------------------------------------------------------------------

	enum PhotometricType {
		PHOTOMETRIC_TYPE_C = 1,
		PHOTOMETRIC_TYPE_B = 2,
		PHOTOMETRIC_TYPE_A = 3
	};

	unsigned int 	m_NumberOfLamps;
	double			m_LumensPerLamp;
	double			m_CandelaMultiplier;
	unsigned int	m_NumberOfVerticalAngles;
	unsigned int	m_NumberOfHorizontalAngles;
	PhotometricType m_PhotometricType;
	unsigned int 	m_UnitsType;
	double			m_LuminaireWidth;
	double			m_LuminaireLength;
	double			m_LuminaireHeight;

	double			BallastFactor;
	double			BallastLampPhotometricFactor;
	double			InputWatts;

	std::vector<double>	m_VerticalAngles; 
	std::vector<double>	m_HorizontalAngles; 

	std::vector<std::vector<double> > m_CandelaValues;

private:
	bool PrivateLoad(std::istream &is);

	bool BuildKeywordList(std::istream &is);
	void BuildDataLine(std::stringstream &, unsigned int, std::vector<double>&);
	bool BuildLightData(std::istream &is);

	void inline ReadLine(std::istream &is, std::string & sLine) {
		memset(&sLine[0], 0, sLine.size());
		is.getline(&sLine[0], sLine.size(), 0x0A);
	}

	bool 			m_bValid;
};

}

#endif // _SLG_PHOTOMETRICDATAIES_H
