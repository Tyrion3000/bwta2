// Copyright (c) 2006-2007  INRIA Sophia-Antipolis (France).
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
// $URL: svn+ssh://scm.gforge.inria.fr/svn/cgal/branches/CGAL-3.5-branch/Surface_mesher/include/CGAL/Multi_surface_3.h $
// $Id: Multi_surface_3.h 37876 2007-04-03 13:59:17Z lrineau $
//
// Author(s)     : Laurent RINEAU

#ifndef CGAL_MULTI_SURFACE_3_H
#define CGAL_MULTI_SURFACE_3_H

namespace CGAL {

  template<
    typename Surface_a,
    typename Surface_b
    >
  class Multi_surface_3 
  {
    const Surface_a& surf_a;
    const Surface_b& surf_b;
  public:
    Multi_surface_3(const Surface_a& surface_a, const Surface_b& surface_b)
      : surf_a(surface_a), surf_b(surface_b)
    {
    }

    const Surface_a& surface_a() const
    {
      return surf_a;
    }
    
      
    const Surface_b& surface_b() const
    {
      return surf_b;
    }
  };
} // end namespace CGAL

#include <CGAL/Surface_mesher/Combining_oracle.h>

namespace CGAL {
  template <typename Surface_a, typename Surface_b>
  struct Surface_mesh_traits_generator_3 <
    Multi_surface_3<Surface_a, Surface_b>
  >
  {
    typedef typename Surface_mesh_traits_generator_3<Surface_a>::type Oracle_a;
    typedef typename Surface_mesh_traits_generator_3<Surface_b>::type Oracle_b;

    typedef typename Surface_mesher::Combining_oracle<
      Oracle_a,
      Oracle_b
      > Type;
    
    typedef Type type; // Boost meta-programming compatibility
  };
} // end namespace CGAL, second occurrence.

#endif // CGAL_MULTI_SURFACE_3_H
