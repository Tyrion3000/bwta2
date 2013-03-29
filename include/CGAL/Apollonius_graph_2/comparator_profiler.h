// Copyright (c) 2003,2006  INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you may redistribute it under
// the terms of the Q Public License version 1.0.
// See the file LICENSE.QPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL: svn+ssh://scm.gforge.inria.fr/svn/cgal/branches/CGAL-3.5-branch/Apollonius_graph_2/include/CGAL/Apollonius_graph_2/comparator_profiler.h $
// $Id: comparator_profiler.h 32634 2006-07-19 21:58:48Z mkaravel $
// 
//
// Author(s)     : Menelaos Karavelas <mkaravel@cse.nd.edu>



#ifndef CGAL_COMPARATOR_PROFILER_H
#define CGAL_COMPARATOR_PROFILER_H

#include <CGAL/Apollonius_graph_2/basic.h>

CGAL_BEGIN_NAMESPACE

CGAL_APOLLONIUS_GRAPH_2_BEGIN_NAMESPACE

class comparator_profiler
{
public:
  static bool count_cases;
  static unsigned long case_1_counter;
  static unsigned long case_2_counter;
  static unsigned long case_3a_Jpos_counter;
  static unsigned long case_3a_Jneg_counter;
  static unsigned long case_3b_Jpos_counter;
  static unsigned long case_3b_Jneg_counter;
  static unsigned long case_4_counter;
  static unsigned long case_5_counter;
  static unsigned long case_degenerate_counter;
public:
  static unsigned long counter_rr;
  static unsigned long counter_rr_p3inf;
  static unsigned long counter_rr_p4;
  static unsigned long counter_rr_e;
  static unsigned long counter_rr_r0;
  //  static unsigned long counter_rr_jneg;

  static void reset()
  {
    count_cases = false;
    case_1_counter = 0;
    case_2_counter = 0;
    case_3a_Jpos_counter = 0;
    case_3a_Jneg_counter = 0;
    case_3b_Jpos_counter = 0;
    case_3b_Jneg_counter = 0;
    case_4_counter = 0;
    case_5_counter = 0;
    case_degenerate_counter = 0;

    counter_rr = 0;
    counter_rr_p3inf = 0;
    counter_rr_p4 = 0;
    counter_rr_e = 0;
    counter_rr_r0 = 0;
    //    counter_rr_jneg = 0;
  }

  template< class FT >
  static void count_case(const FT& a1, const FT& b1, const FT& c1,
			 const FT& a2, const FT& b2, const FT& c2)
  {
    // works correctly only with leda_real
    FT D1 = CGAL::square(b1) - a1 * c1;
    
    FT l1 = (b1 - CGAL::sqrt(D1)) / a1;
    FT r1 = (b1 + CGAL::sqrt(D1)) / a1;
    if ( a1 < 0 ) { std::swap(r1, l1); }
    
    FT D2 = CGAL::square(b2) - a2 * c2;

    if ( D1 == 0 || D2 == 0 ) {
      case_degenerate_counter++;
      return;
    }

    FT l2 = (b2 - CGAL::sqrt(D2)) / a2;
    FT r2 = (b2 + CGAL::sqrt(D2)) / a2;
    if ( a2 < 0 ) { std::swap(r2, l2); }

    if ( l1 < l2 ) {
      if ( r1 > r2 ) {
	FT J = a1 * b2 - a2 * b1;
	if ( J > 0 ) {
	  case_3b_Jpos_counter++;
	} else if ( J < 0 ) {
	  case_3b_Jneg_counter++;
	} else {
	  case_degenerate_counter++;
	}
      } else if ( r1 < r2 ) {
	if ( r1 < l2 ) {
	  case_5_counter++;
	} else if ( r1 > l2 ) {
	  case_4_counter++;
	} else {
	  case_degenerate_counter++;
	}
      } else {
	case_degenerate_counter++;
      }
    } else if ( l1 > l2 ) {
      if ( r1 < r2 ) {
	FT J = a1 * b2 - a2 * b1;
	if ( J > 0 ) {
	  case_3a_Jpos_counter++;
	} else if ( J < 0 ) {
	  case_3a_Jneg_counter++;
	} else {
	  case_degenerate_counter++;
	}
      } else if ( r1 > r2 ) {
	if ( l1 < r2 ) {
	  case_2_counter++;
	} else if ( l1 > r2 ) {
	  case_1_counter++;
	} else {
	  case_degenerate_counter++;
	}
      } else {
	case_degenerate_counter++;
      }
    } else {
      case_degenerate_counter++;
    }
  }
};

bool comparator_profiler::count_cases = false;
unsigned long comparator_profiler::case_1_counter = 0;
unsigned long comparator_profiler::case_2_counter = 0;
unsigned long comparator_profiler::case_3a_Jpos_counter = 0;
unsigned long comparator_profiler::case_3a_Jneg_counter = 0;
unsigned long comparator_profiler::case_3b_Jpos_counter = 0;
unsigned long comparator_profiler::case_3b_Jneg_counter = 0;
unsigned long comparator_profiler::case_4_counter = 0;
unsigned long comparator_profiler::case_5_counter = 0;
unsigned long comparator_profiler::case_degenerate_counter = 0;

unsigned long comparator_profiler::counter_rr = 0;
unsigned long comparator_profiler::counter_rr_p3inf = 0;
unsigned long comparator_profiler::counter_rr_p4 = 0;
unsigned long comparator_profiler::counter_rr_e = 0;
unsigned long comparator_profiler::counter_rr_r0 = 0;
//unsigned long comparator_profiler::counter_rr_jneg = 0;

CGAL_APOLLONIUS_GRAPH_2_END_NAMESPACE

CGAL_END_NAMESPACE

#endif // CGAL_COMPARATOR_PROFILER_H
