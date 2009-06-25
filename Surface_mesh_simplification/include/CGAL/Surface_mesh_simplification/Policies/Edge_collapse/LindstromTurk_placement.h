// Copyright (c) 2006  GeometryFactory (France). All rights reserved.
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
// $URL$
// $Id$
//
// Author(s)     : Fernando Cacciola <fernando.cacciola@geometryfactory.com>
//
#ifndef CGAL_SURFACE_MESH_SIMPLIFICATION_POLICIES_EDGE_COLLAPSE_LINDSTROMTURK_PLACEMENT_H
#define CGAL_SURFACE_MESH_SIMPLIFICATION_POLICIES_EDGE_COLLAPSE_LINDSTROMTURK_PLACEMENT_H 1

#include <CGAL/Surface_mesh_simplification/Detail/Common.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Detail/Lindstrom_Turk_core.h>

CGAL_BEGIN_NAMESPACE

namespace Surface_mesh_simplification  
{

template<class ECM_>
class LindstromTurk_placement
{
public:
    
  typedef ECM_ ECM ;
  
  typedef Edge_profile<ECM> Profile ;
  
  typedef typename halfedge_graph_traits<ECM>::Point Point ;

  typedef optional<Point> result_type ;

public:

  LindstromTurk_placement( LindstromTurk_params const& aParams = LindstromTurk_params() ) : mParams(aParams) {}
     
  result_type operator()( Profile const& aProfile) const
  {
    return LindstromTurkCore<ECM>(mParams,aProfile).compute_placement() ;
  }
  
private:

  LindstromTurk_params mParams ;    

};


} // namespace Surface_mesh_simplification

CGAL_END_NAMESPACE

#endif // CGAL_SURFACE_MESH_SIMPLIFICATION_POLICIES_EDGE_COLLAPSE_LINDSTROMTURK_PLACEMENT_H //
// EOF //
 
