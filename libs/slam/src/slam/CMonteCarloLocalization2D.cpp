/* +---------------------------------------------------------------------------+
   |          The Mobile Robot Programming Toolkit (MRPT) C++ library          |
   |                                                                           |
   |                       http://www.mrpt.org/                                |
   |                                                                           |
   |   Copyright (C) 2005-2011  University of Malaga                           |
   |                                                                           |
   |    This software was written by the Machine Perception and Intelligent    |
   |      Robotics Lab, University of Malaga (Spain).                          |
   |    Contact: Jose-Luis Blanco  <jlblanco@ctima.uma.es>                     |
   |                                                                           |
   |  This file is part of the MRPT project.                                   |
   |                                                                           |
   |     MRPT is free software: you can redistribute it and/or modify          |
   |     it under the terms of the GNU General Public License as published by  |
   |     the Free Software Foundation, either version 3 of the License, or     |
   |     (at your option) any later version.                                   |
   |                                                                           |
   |   MRPT is distributed in the hope that it will be useful,                 |
   |     but WITHOUT ANY WARRANTY; without even the implied warranty of        |
   |     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         |
   |     GNU General Public License for more details.                          |
   |                                                                           |
   |     You should have received a copy of the GNU General Public License     |
   |     along with MRPT.  If not, see <http://www.gnu.org/licenses/>.         |
   |                                                                           |
   +---------------------------------------------------------------------------+ */

#include <mrpt/slam.h>  // Precompiled headers

#include <mrpt/slam/CMonteCarloLocalization2D.h>

#include <mrpt/utils/CTicTac.h>
#include <mrpt/slam/COccupancyGridMap2D.h>
#include <mrpt/slam/CActionCollection.h>
#include <mrpt/slam/CSensoryFrame.h>

#include <mrpt/random.h>
#include <mrpt/math.h>

#include <mrpt/slam/PF_aux_structs.h>

using namespace mrpt;
using namespace mrpt::bayes;
using namespace mrpt::poses;
using namespace mrpt::math;
using namespace mrpt::slam;
using namespace mrpt::random;
using namespace mrpt::utils;
using namespace std;

#include <mrpt/slam/PF_implementations_data.h>

namespace mrpt
{
	namespace slam
	{
		/** Fills out a "TPoseBin2D" variable, given a path hypotesis and (if not set to NULL) a new pose appended at the end, using the KLD params in "options". */
		template <>
		void KLF_loadBinFromParticle(
			mrpt::slam::detail::TPoseBin2D &outBin,
			const TKLDParams  	&opts,
			const CMonteCarloLocalization2D::CParticleDataContent 	*currentParticleValue,
			const TPose3D		*newPoseToBeInserted)
		{
			// 2D pose approx: Use the latest pose only:
			if (newPoseToBeInserted)
			{
				outBin.x 	= round( newPoseToBeInserted->x / opts.KLD_binSize_XY );
				outBin.y	= round( newPoseToBeInserted->y / opts.KLD_binSize_XY );
				outBin.phi	= round( newPoseToBeInserted->yaw / opts.KLD_binSize_PHI );
			}
			else
			{
				ASSERT_(currentParticleValue)
				outBin.x 	= round( currentParticleValue->x() / opts.KLD_binSize_XY );
				outBin.y	= round( currentParticleValue->y() / opts.KLD_binSize_XY );
				outBin.phi	= round( currentParticleValue->phi() / opts.KLD_binSize_PHI );
			}
		}
	}
}

#include <mrpt/slam/PF_implementations.h>

//#if defined(_MSC_VER)
//#	pragma warning(push)
//#	pragma warning(disable:4355) // for the "this" argument below
//#endif

/*---------------------------------------------------------------
				ctor
 ---------------------------------------------------------------*/
// Passing a "this" pointer at this moment is not a problem since it will be NOT access until the object is fully initialized
CMonteCarloLocalization2D::CMonteCarloLocalization2D( size_t M ) :
	CPosePDFParticles(M)
//	PF_implementation<CPose2D>(static_cast<mrpt::bayes::CParticleFilterData<CPose2D>&>(*this),static_cast<mrpt::bayes::CParticleFilterCapable&>(*this) )
{
}

//#if defined(_MSC_VER)
//#	pragma warning(pop)
//#endif

/*---------------------------------------------------------------
				Dtor
 ---------------------------------------------------------------*/
CMonteCarloLocalization2D::~CMonteCarloLocalization2D()
{
}


/*---------------------------------------------------------------
						getLastPose
 ---------------------------------------------------------------*/
const TPose3D* CMonteCarloLocalization2D::getLastPose(const size_t i) const
{
	if (i>=m_particles.size()) THROW_EXCEPTION("Particle index out of bounds!");
	static TPose3D auxHolder;
	ASSERTDEB_(m_particles[i].d!=NULL)
	auxHolder = TPose3D( TPose2D(*m_particles[i].d));
	return &auxHolder;
}



/*---------------------------------------------------------------

			prediction_and_update_pfStandardProposal

 ---------------------------------------------------------------*/
void  CMonteCarloLocalization2D::prediction_and_update_pfStandardProposal(
	const mrpt::slam::CActionCollection	* actions,
	const mrpt::slam::CSensoryFrame		* sf,
	const bayes::CParticleFilter::TParticleFilterOptions &PF_options )
{
	MRPT_START

	if (sf)
	{	// A map MUST be supplied!
		ASSERT_(options.metricMap || options.metricMaps.size()>0)
		if (!options.metricMap)
			ASSERT_(options.metricMaps.size() == m_particles.size() )
	}

	PF_SLAM_implementation_pfStandardProposal<mrpt::slam::detail::TPoseBin2D>(actions, sf, PF_options,options.KLD_params);

	MRPT_END
}

/*---------------------------------------------------------------

			prediction_and_update_pfAuxiliaryPFStandard

 ---------------------------------------------------------------*/
void  CMonteCarloLocalization2D::prediction_and_update_pfAuxiliaryPFStandard(
	const mrpt::slam::CActionCollection	* actions,
	const mrpt::slam::CSensoryFrame		* sf,
	const bayes::CParticleFilter::TParticleFilterOptions &PF_options )
{
	MRPT_START

	if (sf)
	{	// A map MUST be supplied!
		ASSERT_(options.metricMap || options.metricMaps.size()>0)
		if (!options.metricMap)
			ASSERT_(options.metricMaps.size() == m_particles.size() )
	}

	PF_SLAM_implementation_pfAuxiliaryPFStandard<mrpt::slam::detail::TPoseBin2D>(actions, sf, PF_options,options.KLD_params);

	MRPT_END
}


/*---------------------------------------------------------------

			prediction_and_update_pfAuxiliaryPFOptimal

 ---------------------------------------------------------------*/
void  CMonteCarloLocalization2D::prediction_and_update_pfAuxiliaryPFOptimal(
	const mrpt::slam::CActionCollection	* actions,
	const mrpt::slam::CSensoryFrame		* sf,
	const bayes::CParticleFilter::TParticleFilterOptions &PF_options )
{
	MRPT_START

	if (sf)
	{	// A map MUST be supplied!
		ASSERT_(options.metricMap || options.metricMaps.size()>0)
		if (!options.metricMap)
			ASSERT_(options.metricMaps.size() == m_particles.size() )
	}

	PF_SLAM_implementation_pfAuxiliaryPFOptimal<mrpt::slam::detail::TPoseBin2D>(actions, sf, PF_options,options.KLD_params);

	MRPT_END
}


/*---------------------------------------------------------------
			PF_SLAM_computeObservationLikelihoodForParticle
 ---------------------------------------------------------------*/
double CMonteCarloLocalization2D::PF_SLAM_computeObservationLikelihoodForParticle(
	const CParticleFilter::TParticleFilterOptions	&PF_options,
	const size_t			particleIndexForMap,
	const CSensoryFrame		&observation,
	const CPose3D			&x ) const
{
	ASSERT_( options.metricMap || particleIndexForMap<options.metricMaps.size() )

	CMetricMap *map = (options.metricMap) ?
		options.metricMap :  // All particles, one map
		options.metricMaps[particleIndexForMap]; // One map per particle

	// For each observation:
	double ret = 1;
	for (CSensoryFrame::const_iterator it=observation.begin();it!=observation.end();++it)
		ret += map->computeObservationLikelihood( it->pointer(), x );	// Compute the likelihood:

	// Done!
	return ret;
}

// Specialization for my kind of particles:
void CMonteCarloLocalization2D::PF_SLAM_implementation_custom_update_particle_with_new_pose(
	CPose2D *particleData,
	const TPose3D &newPose) const
{
	*particleData = CPose2D( TPose2D(newPose) );
}


void CMonteCarloLocalization2D::PF_SLAM_implementation_replaceByNewParticleSet(
	CParticleList	&old_particles,
	const vector<TPose3D>	&newParticles,
	const vector_double		&newParticlesWeight,
	const vector<size_t>	&newParticlesDerivedFromIdx )  const
{
	ASSERT_EQUAL_(size_t(newParticlesWeight.size()),size_t(newParticles.size()))

	// ---------------------------------------------------------------------------------
	// Substitute old by new particle set:
	//   Old are in "m_particles"
	//   New are in "newParticles", "newParticlesWeight","newParticlesDerivedFromIdx"
	// ---------------------------------------------------------------------------------
	// Free old m_particles:
	for (size_t i=0;i<old_particles.size();i++)
			mrpt::utils::delete_safe( old_particles[ i ].d );

	// Copy into "m_particles"
	const size_t N = newParticles.size();
	old_particles.resize(N);
	for (size_t i=0;i<N;i++)
	{
		old_particles[i].log_w = newParticlesWeight[i];
		old_particles[i].d = new CPose2D( TPose2D( newParticles[i] ));
	}
}

void  CMonteCarloLocalization2D::resetUniformFreeSpace(
	COccupancyGridMap2D		*theMap,
	const double 					freeCellsThreshold ,
	const int	 					particlesCount ,
	const double 					x_min ,
	const double 					x_max ,
	const double 					y_min ,
	const double 					y_max ,
	const double 					phi_min,
	const double 					phi_max)
{
	MRPT_START

	ASSERT_(theMap!=NULL)

	int					sizeX = theMap->getSizeX();
	int					sizeY = theMap->getSizeY();
	double				gridRes = theMap->getResolution();
	std::vector<double>	freeCells_x,freeCells_y;
	size_t				nFreeCells;
	unsigned int		xIdx1,xIdx2;
	unsigned int		yIdx1,yIdx2;

	freeCells_x.reserve( sizeX * sizeY );
	freeCells_y.reserve( sizeX * sizeY );

	if (x_min>theMap->getXMin())
			xIdx1 = max(0, theMap->x2idx( x_min ) );
	else	xIdx1 = 0;
	if (x_max<theMap->getXMax())
			xIdx2 = min(sizeX-1, theMap->x2idx( x_max ) );
	else	xIdx2 = sizeX-1;
	if (y_min>theMap->getYMin())
			yIdx1 = max(0, theMap->y2idx( y_min ) );
	else	yIdx1 = 0;
	if (y_max<theMap->getYMax())
			yIdx2 = min(sizeY-1, theMap->y2idx( y_max ) );
	else	yIdx2 = sizeY-1;


	for (unsigned int x=xIdx1;x<=xIdx2;x++)
		for (unsigned int y=yIdx1;y<=yIdx2;y++)
			if (theMap->getCell(x,y)>=freeCellsThreshold)
			{
				freeCells_x.push_back(theMap->idx2x(x));
				freeCells_y.push_back(theMap->idx2y(y));
			}

	nFreeCells = freeCells_x.size();

	// Assure that map is not fully occupied!
	ASSERT_( nFreeCells );


	if (particlesCount>0)
	{
		clear();
		m_particles.resize(particlesCount);
		for (int i=0;i<particlesCount;i++)
			m_particles[i].d = new CPose2D();
	}

	const size_t M = m_particles.size();

	// Generate pose m_particles:
	for (size_t i=0;i<M;i++)
	{
		int idx = round(randomGenerator.drawUniform(0.0,nFreeCells-1.001));

		m_particles[i].d->x( freeCells_x[idx] + randomGenerator.drawUniform( -gridRes, gridRes ) );
		m_particles[i].d->y( freeCells_y[idx] + randomGenerator.drawUniform( -gridRes, gridRes ) );
		m_particles[i].d->phi( randomGenerator.drawUniform( phi_min, phi_max ) );
		m_particles[i].log_w=0;
	}

	MRPT_END
}


