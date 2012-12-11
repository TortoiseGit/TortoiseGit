/*
 * $Revision: 2614 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-16 11:30:08 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementations of a collection of classes used to drive
 * Coin.
 *
 * \author Markus Chimani
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.txt in the root directory of the OGDF installation for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * \see  http://www.gnu.org/copyleft/gpl.html
 ***************************************************************/

#ifdef USE_COIN

#include <ogdf/external/coin.h>
#include <coin/CoinPackedVector.hpp>
#include <coin/OsiCuts.hpp>
#include <ogdf/basic/Logger.h>

#ifdef COIN_OSI_CPX
	#include <coin/OsiCpxSolverInterface.hpp> // CPLEX
	#include "cplex.h"
	#include <ogdf/basic/tuples.h>
#elif COIN_OSI_SYM
	#include <coin/OsiSymSolverInterface.hpp> // Symphony
#elif  COIN_OSI_CLP
	#include <coin/OsiClpSolverInterface.hpp> // Coin-OR LP
#else
	#error "Compiler-flag USE_COIN requires an additional COIN_OSI_xxx-flag to select the LP solver backend."
#endif

namespace ogdf {

#ifdef COIN_OSI_CPX

	int CPXPUBLIC CPX_CutCallback(CPXCENVptr xenv, void *cbdata,
			int wherefrom, void *cbhandle, int *useraction_p) {
//		cout << "Entering CPX Callback\n" << flush;
		CPXLPptr nodelp;
		CPXgetcallbacknodelp(xenv, cbdata, wherefrom, &nodelp);

		CoinCallbacks* ccc = (CoinCallbacks*)cbhandle;

		int length = CPXgetnumcols(xenv,nodelp) - 1; //hey, don't ask me! some VERY WIERD PHENOMENON... crap
		double objVal;
		double* solution = new double[length];
		CPXgetcallbacknodeobjval(xenv, cbdata, wherefrom, &objVal);
		CPXgetcallbacknodex(xenv, cbdata, wherefrom, solution, 0, length-1);

		OsiCuts* cuts = new OsiCuts();
		CoinCallbacks::CutReturn ret = ccc->cutCallback(objVal, solution, cuts);

		if(ret == CoinCallbacks::CR_AddCuts) {
			for(int i = cuts->sizeRowCuts(); i-->0;) {
				const OsiRowCut& c = cuts->rowCut(i);
				const CoinPackedVector& vec = c.row();

				if(c.globallyValid())
					/* Old Cplex-Versions did NOT have the last parameter (now set to "false").
					 * If you compile agains an older CPLEX version, simple *REMOVE*
					 *   ", false"
					 * from the calls to CPXcutscallbackadd
					 */
					CPXcutcallbackadd(xenv, cbdata, wherefrom,
						vec.getNumElements(), c.rhs(), c.sense(), vec.getIndices(), vec.getElements(), false);  //default to non-purgable cuts
				else
					CPXcutcallbackaddlocal(xenv, cbdata, wherefrom,
						vec.getNumElements(), c.rhs(), c.sense(), vec.getIndices(), vec.getElements());
				cuts->eraseRowCut(i);
			}
			if(cuts->sizeColCuts() > 0) {
				cerr << "ColCuts currently not supported...\n";
				OGDF_THROW_PARAM(LibraryNotSupportedException, lnscFunctionNotImplemented);
			}
		}

		*useraction_p =
			( ret == CoinCallbacks::CR_Error) ? CPX_CALLBACK_FAIL :
				( ret == CoinCallbacks::CR_AddCuts ) ? CPX_CALLBACK_SET : CPX_CALLBACK_DEFAULT;
		delete cuts;
		delete[] solution;
//		cout << "Leaving CPX Callback\n" << flush;
		return 0; // success
	}

	int CPXPUBLIC CPX_HeuristicCallback (CPXCENVptr env, void *cbdata, int wherefrom,
			void *cbhandle, double *objval_p, double *x, int *checkfeas_p, int *useraction_p) {
		CoinCallbacks* ccc = (CoinCallbacks*)cbhandle;
		CoinCallbacks::HeuristicReturn ret = ccc->heuristicCallback(*objval_p, x);
		*checkfeas_p = 0; // no check. callback has to ensure that new solution (if any) is integer feasible
		switch(ret) {
			case CoinCallbacks::HR_Error:
				*useraction_p = CPX_CALLBACK_FAIL;
			break;
			case CoinCallbacks::HR_Ignore:
				*useraction_p = CPX_CALLBACK_DEFAULT;
			break;
			case CoinCallbacks::HR_Update:
				*useraction_p = CPX_CALLBACK_SET;
			break;
			default:
				OGDF_THROW_PARAM(LibraryNotSupportedException, lnscFunctionNotImplemented);
		}
		return 0;
	}

	int CPXPUBLIC CPX_IncumbentCallback (CPXCENVptr env, void *cbdata, int wherefrom,
			void *cbhandle, double objval, double *x, int *isfeas_p, int *useraction_p) {
		CoinCallbacks* ccc = (CoinCallbacks*)cbhandle;
		CoinCallbacks::IncumbentReturn ret = ccc->incumbentCallback(objval, x);
		switch(ret) {
			case CoinCallbacks::IR_Error:
				*useraction_p = CPX_CALLBACK_FAIL;
			break;
			case CoinCallbacks::IR_Update:
				*isfeas_p = 1;
				*useraction_p = CPX_CALLBACK_SET;
			break;
			case CoinCallbacks::IR_Ignore:
				*isfeas_p = 0;
				*useraction_p = CPX_CALLBACK_SET;
			break;
			default:
				OGDF_THROW_PARAM(LibraryNotSupportedException, lnscFunctionNotImplemented);
		}
		return 0;
	}
/*
	int CPXPUBLIC CPX_BranchCallback (CPXCENVptr env, void *cbdata, int wherefrom, void *cbhandle,
			int type, int  sos, int nodecnt, int bdcnt, double *nodeest, int *nodebeg, int *indices,
			char *lu, int *bd, int *useraction_p) {
		CoinCallbacks* ccc = (CoinCallbacks*)cbhandle;
		CoinCallbacks::BranchReturn ret = ccc->branchCallback(objVal, x, ...); // callbacks to setbounds etc...

		switch(ret) {
			case CoinCallbacks::BR_Error:
				*useraction_p = CPX_CALLBACK_FAIL;
			break;
			case CoinCallbacks::BR_Take:
				*infeas_p = 1;
				*useraction_p = CPX_CALLBACK_SET;
			break;
			case CoinCallbacks::BR_ThrowAway:
				*infeas_p = 0;
				*useraction_p = CPX_CALLBACK_SET;
			break;
			default:
				OGDF_THROW_PARAM(LibraryNotSupportedException, lnscFunctionNotImplemented);
		}
		return 0;
	}
	*/

#endif // COIN_OSI_CPX

	OsiSolverInterface* CoinManager::createCorrectOsiSolverInterface() {
		OsiSolverInterface* ret = new
		#ifdef COIN_OSI_CPX
			OsiCpxSolverInterface(); // CPLEX
		#elif COIN_OSI_SYM
			OsiSymSolverInterface(); // Symphony
		#else // COIN_OSI_CLP
			OsiClpSolverInterface(); // Coin-OR LP
		#endif
		logging(ret, !Logger::globalStatisticMode() && Logger::globalLogLevel() <= Logger::LL_MINOR);
		return ret;
	}

	void CoinManager::logging(OsiSolverInterface* osi, bool logMe) {
		osi->messageHandler()->setLogLevel(logMe ? 1 : 0);
	}

	bool CoinCallbacks::registerCallbacks(OsiSolverInterface* posi, int callbackTypes) {
	#ifdef COIN_OSI_CPX
		OsiCpxSolverInterface* x = dynamic_cast<OsiCpxSolverInterface*>(posi);
		CPXENVptr envptr = x->getEnvironmentPtr();
		CPXLPptr lpptr = x->getLpPtr();
		if(callbackTypes & CT_Cut)
			CPXsetcutcallbackfunc(envptr, &CPX_CutCallback, this);
		if(callbackTypes & CT_Heuristic)
			CPXsetheuristiccallbackfunc(envptr, &CPX_HeuristicCallback, this);
		if(callbackTypes & CT_Incumbent)
			CPXsetincumbentcallbackfunc(envptr, &CPX_IncumbentCallback, this);
//		if(callbackTypes & CT_Branch)
//			CPXsetbranchcallbackfunc(envptr, &CPX_BranchCallback, this);

		CPXsetintparam(envptr, CPX_PARAM_MIPCBREDLP, CPX_OFF);

		CPXsetintparam(envptr, CPX_PARAM_PRELINEAR, 0);
		CPXsetintparam(envptr, CPX_PARAM_HEURFREQ, -1);
		CPXsetintparam(envptr, CPX_PARAM_PREIND, 0);
		CPXsetintparam(envptr, CPX_PARAM_BNDSTRENIND, 0);
		CPXsetintparam(envptr, CPX_PARAM_AGGIND, 0);
		CPXsetintparam(envptr, CPX_PARAM_COEREDIND, 0);
		CPXsetintparam(envptr, CPX_PARAM_RELAXPREIND, 0);
		CPXsetintparam(envptr, CPX_PARAM_PREPASS, 0);
	//	CPXsetintparam(envptr, CPX_PARAM_REPEATPRESOLVE, 0); // only exists on cplex10
		CPXsetintparam(envptr, CPX_PARAM_REDUCE, 0);

		return true;
	#else
		//#warning "CoinCallbacks disabled. Currently only applicable for CPLEX. I'm sorry."
		return false;
	#endif
	}

}

#endif // USE_COIN
