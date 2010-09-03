/*
 * sci_jsbsimComm.cpp
 * Copyright (C) James Goppert 2010 <jgoppert@users.sourceforge.net>
 *
 * sci_jsbsimComm.cpp is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * sci_jsbsimComm.cpp is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FGFDMExec.h"
#include "models/FGFCS.h"
#include "Trim.h"
#include "FGStateSpace.h"
#include <iostream>
#include <string>

class JSBSimComm
{
public:
	JSBSimComm() : ss(m_fdm)
	{
		using namespace JSBSim;
		m_fdm.SetDebugLevel(0);
		m_fdm.Setdt(1./120);
 		std::cout << "initializing" << std::endl;
		m_fdm.LoadModel("../aircraft","../engine","../systems","f16");

		// defaults
		bool variablePropPitch = false;

		// get propulsion pointer to determine type/ etc.
		FGEngine * engine0 = m_fdm.GetPropulsion()->GetEngine(0);
		FGThruster * thruster0 = engine0->GetThruster();

		// state space
		ss.x.add(new FGStateSpace::Vt);
		ss.x.add(new FGStateSpace::Alpha);
		ss.x.add(new FGStateSpace::Theta);
		ss.x.add(new FGStateSpace::Q);

		if (thruster0->GetType()==FGThruster::ttPropeller)
		{
			ss.x.add(new FGStateSpace::Rpm);
			if (variablePropPitch) ss.x.add(new FGStateSpace::Pitch);
		}
		switch (engine0->GetType())
		{
		case FGEngine::etTurbine:
			ss.x.add(new FGStateSpace::N2);
			break;
		case FGEngine::etTurboprop:
			ss.x.add(new FGStateSpace::N1);
			break;
		default:
			break;
		}
		ss.x.add(new FGStateSpace::Beta);
		ss.x.add(new FGStateSpace::Phi);
		ss.x.add(new FGStateSpace::P);
		ss.x.add(new FGStateSpace::R);

		ss.x.add(new FGStateSpace::ThrottlePos);
		ss.x.add(new FGStateSpace::DaPos);
		ss.x.add(new FGStateSpace::DePos);
		ss.x.add(new FGStateSpace::DrPos);

		ss.u.add(new FGStateSpace::ThrottleCmd);
		ss.u.add(new FGStateSpace::DaCmd);
		ss.u.add(new FGStateSpace::DeCmd);
		ss.u.add(new FGStateSpace::DrCmd);

		// state feedback
		ss.y = ss.x;
	}
	JSBSim::FGStateSpace ss;
private:
	JSBSim::FGFDMExec m_fdm;
};

extern "C"
{

#include <scicos/scicos_block4.h>
#include <math.h>
#include "definitions.hpp"

void sci_jsbsimComm(scicos_block *block, scicos::enumScicosFlags flag)
{
	//definitions
	static JSBSimComm comm;

	// data
	double *u=(double*)GetInPortPtrs(block,1);
	double *xOut=(double*)GetOutPortPtrs(block,1);
	double *y=(double*)GetOutPortPtrs(block,2);
	double *x=(double*)GetState(block);
	double *xd=(double*)GetDerState(block);

	//handle flags
	if (flag==scicos::initialize || flag==scicos::reinitialize)
	{
		std::cout << "initializing" << std::endl;
		sci_jsbsimComm(block,scicos::updateState);
		std::cin.get();
	}
	else if (flag==scicos::terminate)
	{
		std::cout << "terminating" << std::endl;
	}
	else if (flag==scicos::updateState)
	{
		comm.ss.u.set(u);
		comm.ss.x.set(x);
		std::cout << "updating state" << std::endl;
		std::cout << comm.ss << std::endl;
	}
	else if (flag==scicos::computeDeriv)
	{
		//comm.ss.x.getDeriv(xd);
		std::cout << "computing deriv" << std::endl;
		std::cout << comm.ss.x.getDeriv() << std::endl;
	}
	else if (flag==scicos::computeOutput)
	{
		comm.ss.x.get(xOut);
		comm.ss.y.get(y);
		std::cout << "computing output" << std::endl;
	}
	else
	{
		std::cout << "unhandled flag: " << flag << std::endl;
	}
}

} // extern c

// vim:ts=4:sw=4