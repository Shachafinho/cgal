// Copyright (c) 2017 Tel-Aviv University (Israel).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Author(s)     : Shachaf Ben Jakov <shachafinho@gmail.com>
//                 Efi Fogel <efif@post.tau.ac.il>

#ifndef CGAL_ARR_REFLECT_H
#define CGAL_ARR_REFLECT_H

#include <CGAL/license/Arrangement_on_surface_2.h>

/*! \file
 * Definition of the global Arr_reflect() function.
 */

#include <list>
#include <iterator>
#include <vector>

#include <boost/optional/optional.hpp>
#include <boost/type_traits.hpp>

#include <CGAL/Arrangement_on_surface_2.h>
#include <CGAL/Arr_accessor.h>
#include <CGAL/assertions.h>

namespace CGAL {

/*! Determines at compile-time whether T has a nested type named "Reflect_2".
 * If so, 'value' holds true, and false otherwise.
 */
template <typename T>
struct has_Reflect_2
{
private:
  // The following types must differ in size.
  typedef char yes[1];
  typedef char no[2];

  // Overloads for when U does or doesn't have the Reflect_2 functor,
  // respectively.
  template <typename U> static yes& check(typename U::Reflect_2*);
  template <typename U> static no& check(...);

public:
  // True iff the first overload worked and T has a nested type named
  // "Reflect_2".
  static const bool value = sizeof(check<T>(nullptr)) == sizeof(yes);
};

template <typename GeomeTraits, typename TopolTraits>
void
reflect_arrangement_impl(Arrangement_on_surface_2<GeomeTraits, TopolTraits>& arr,
                         bool should_merge, boost::mpl::false_)
{
  typedef Arrangement_on_surface_2<GeomeTraits, TopolTraits>
                                                        Arrangement_2;
  typedef Arr_accessor<Arrangement_2>                   Arr_accessor;
  typedef typename Arrangement_2::Vertex_iterator       Vertex_iterator;
  typedef typename Arrangement_2::Edge_iterator         Edge_iterator;
  typedef typename Arrangement_2::X_monotone_curve_2    X_monotone_curve_2;
  typedef typename GeomeTraits::Reflect_2               Reflect_2;
  typedef typename TopolTraits::Dcel                    Dcel;
  typedef typename Dcel::Edge_iterator                  DEdge_iterator;

  // Obtain an accessor to modify the geometric traits.
  Arr_accessor accessor(arr);

  // Obtain a reflect object to reflect the geometric objects.
  Reflect_2 reflect_object = arr.geometry_traits()->reflect_2_object();

  // Handle vertices.
  for (Vertex_iterator vit = arr.vertices_begin(); vit != arr.vertices_end();
       ++vit)
  {
    // Reflect the point associated with the vertex.
    if (!vit->is_at_open_boundary()) {
      accessor.modify_vertex_ex(vit, reflect_object(vit->point()));
    }

    // Update the reflected vertex boundary conditions.
    Arr_parameter_space ps_x = vit->parameter_space_in_x();
    Arr_parameter_space ps_y = vit->parameter_space_in_y();

    Arr_parameter_space ref_ps_x = ps_x, ref_ps_y = ps_y;
    if (ps_x == ARR_LEFT_BOUNDARY) ref_ps_x = ARR_RIGHT_BOUNDARY;
    else if (ps_x == ARR_RIGHT_BOUNDARY) ref_ps_x = ARR_LEFT_BOUNDARY;
    if (ps_y == ARR_BOTTOM_BOUNDARY) ref_ps_y = ARR_TOP_BOUNDARY;
    else if (ps_y == ARR_TOP_BOUNDARY) ref_ps_y = ARR_BOTTOM_BOUNDARY;

    accessor.set_vertex_boundary(vit, ref_ps_x, ref_ps_y);
  }

  // Handle edges.
  for (Edge_iterator eit = arr.edges_begin(); eit != arr.edges_end(); ++eit) {
    // Reflect the curve assoicated with the edge.
    X_monotone_curve_2 reflected_curve;
    reflect_object(eit->curve(), &reflected_curve);
    eit->curve() = reflected_curve;
  }

  // Invert the directions of the edges.
  Dcel& dcel = arr.topology_traits()->dcel();
  for (DEdge_iterator eit = dcel.edges_begin(); eit != dcel.edges_end(); ++eit) {
    eit->set_direction(eit->opposite()->direction());
  }

  // Update the dcel.
  accessor.dcel_updated();
}

template <typename GeomeTraits, typename TopolTraits>
void
reflect_arrangement_impl(Arrangement_on_surface_2<GeomeTraits, TopolTraits>& arr,
                         bool should_merge, boost::mpl::true_)
{
  typedef Arrangement_on_surface_2<GeomeTraits, TopolTraits>
                                                        Arrangement_2;
  typedef Arr_accessor<Arrangement_2>                   Arr_accessor;

  // Arrangement typedefs
  typedef typename Arrangement_2::Point_2               Point_2;
  typedef typename Arrangement_2::X_monotone_curve_2    X_monotone_curve_2;
  typedef typename Arrangement_2::Vertex_iterator       Vertex_iterator;
  typedef typename Arrangement_2::Edge_iterator         Edge_iterator;
  typedef typename Arrangement_2::Vertex_handle         Vertex_handle;
  typedef typename Arrangement_2::Halfedge_handle       Halfedge_handle;

  // Geometry traits typedefs
  typedef typename GeomeTraits::Reflect_2               Reflect_2;
  typedef typename GeomeTraits::Equal_2                 Equal_2;
  typedef typename GeomeTraits::Parameter_space_in_y_2  Parameter_space_in_y_2;
  typedef typename GeomeTraits::Is_on_y_identification_2
    Is_on_y_identification_2;
  typedef typename GeomeTraits::Are_mergeable_2         Are_mergeable_2;
  typedef typename GeomeTraits::Merge_2                 Merge_2;

  // Topology traits typedefs
  typedef typename TopolTraits::Dcel                    Dcel;
  typedef typename TopolTraits::Vertex                  DVertex;
  typedef typename TopolTraits::Halfedge                DHalfedge;
  typedef typename TopolTraits::Face                    DFace;
  typedef typename TopolTraits::Outer_ccb               DOuter_ccb;
  typedef typename TopolTraits::Inner_ccb               DInner_ccb;
  typedef typename Dcel::Edge_iterator                  DEdge_iterator;
  typedef typename Dcel::Face_iterator                  DFace_iterator;
  typedef typename DFace::Inner_ccb_iterator            DInner_ccb_iterator;
  typedef typename DFace::Outer_ccb_iterator            DOuter_ccb_iterator;

  // Obtain an accessor to modify the geometric traits.
  Arr_accessor accessor(arr);

  // Obtain a reflect object to reflect the geometric objects.
  Reflect_2 reflect_object = arr.geometry_traits()->reflect_2_object();
  // Obtain an equal object to check equality of geometric objects.
  Equal_2 equal_object = arr.geometry_traits()->equal_2_object();

  // Obtain objects to determine a vertex boundary conditions.
  Parameter_space_in_y_2 parameter_space_in_y_2 =
    arr.geometry_traits()->parameter_space_in_y_2_object();
  Is_on_y_identification_2 is_on_y_identification_2 =
    arr.geometry_traits()->is_on_y_identification_2_object();

  // Acquire the polar faces so they can be fixed later on.
  DFace* spherical_face = arr.topology_traits()->spherical_face();
  DFace* south_face = arr.topology_traits()->south_face();

  // Handle vertices.
  for (Vertex_iterator vit = arr.vertices_begin(); vit != arr.vertices_end();
       ++vit)
  {
    // Reflect the point associated with the vertex.
    if (!vit->is_at_open_boundary()) {
      accessor.modify_vertex_ex(vit, reflect_object(vit->point()));
    }

    // Update the reflected vertex boundary conditions.
    const Point_2& p = vit->point();
    accessor.set_vertex_boundary(vit,
                                 is_on_y_identification_2(p) ?
                                 ARR_LEFT_BOUNDARY : ARR_INTERIOR,
                                 parameter_space_in_y_2(p));
  }

  typedef std::pair<Halfedge_handle, X_monotone_curve_2>    Split_pair;
  typedef std::vector<Split_pair>                           Split_pairs;
  Split_pairs remaining_curves;

  // Handle edges.
  for (Edge_iterator eit = arr.edges_begin(); eit != arr.edges_end(); ++eit) {
    // Reflect the curve assoicated with the edge.
    std::list<X_monotone_curve_2> x_monotone_curves;
    reflect_object(eit->curve(), std::back_inserter(x_monotone_curves));
    eit->curve() = x_monotone_curves.front();

    // Note that the curve may be divided, resulting in two reflected x-monotone
    // curves.
    CGAL_precondition(x_monotone_curves.size() <= 2);
    if (1 < x_monotone_curves.size()) {
      // Reflecting the original curve divided it into 2 curves.
      // Associate the current halfedge (or its twin) with the second reflected
      // curve so we can insert it later.
      X_monotone_curve_2 additional_curve = x_monotone_curves.back();
      Halfedge_handle hh =
        equal_object(eit->source()->point(), eit->curve().source()) ?
        eit : eit->twin();
      remaining_curves.push_back(std::make_pair(hh, additional_curve));
    }
  }

  // Add the remaining reflected curves by splitting their associated halfedges.
  for (typename Split_pairs::iterator it = remaining_curves.begin();
       it != remaining_curves.end(); ++it)
  {
    Halfedge_handle hh = it->first;

    // Create the shared vertex and set its boundary conditions.
    Vertex_handle vh = accessor.create_vertex(it->second.source());
    accessor.set_vertex_boundary(vh, ARR_LEFT_BOUNDARY, ARR_INTERIOR);

    // Split the halfedge at the shared vertex, setting the new inserted curve
    // as the successor of the already set one.
    accessor.split_edge_ex(hh, vh, hh->curve(), it->second);
  }

  // Merge edges sharing a vertex of degree 2, the point of which was reflected
  // from the boundary.
  if (should_merge) {
    typedef std::vector<Vertex_handle>                  Vertex_handles;
    Vertex_handles mergeable_vertices;

    // Identify mergeable vertices.
    Are_mergeable_2 are_mergeable_object =
      arr.geometry_traits()->are_mergeable_2_object();
    for (Vertex_iterator vit = arr.vertices_begin(); vit != arr.vertices_end();
         ++vit)
    {
      // Check whether target vertex has degree of 2.
      if (2 != vit->degree()) continue;

      // Check whether the incident curves are mergable.
      Halfedge_handle hh1 = vit->incident_halfedges();
      Halfedge_handle hh2 = hh1->next();
      if (!are_mergeable_object(hh1->curve(), hh2->curve())) continue;

      // Check whether the point lied on the boundary before the reflection.
      if (!is_on_y_identification_2(reflect_object(vit->point()))) continue;

      mergeable_vertices.push_back(vit);
    }

    // Merge identified vertices.
    Merge_2 merge_object = arr.geometry_traits()->merge_2_object();
    for (typename Vertex_handles::iterator it = mergeable_vertices.begin();
         it != mergeable_vertices.end(); ++it)
    {
      Halfedge_handle hh1 = (*it)->incident_halfedges();
      Halfedge_handle hh2 = hh1->next();

      X_monotone_curve_2 merged_xcv;
      merge_object(hh1->curve(), hh2->curve(), merged_xcv);
      arr.merge_edge(hh1, hh2, merged_xcv);
    }
  }

  // Reflect the topology-specific structures.
  Dcel& dcel = arr.topology_traits()->dcel();

  // Flip h and its opposite, and update the incidence records.
  for (DEdge_iterator eit = dcel.halfedges_begin(); eit != dcel.halfedges_end();
       ++eit)
  {
    DHalfedge* h = &(*eit);
    DHalfedge* opp_h = h->opposite();
    DVertex* v = h->vertex();
    DVertex* opp_v = opp_h->vertex();

    // Swap targets between h and its opposite, and update their direction
    // accordingly.
    h->set_vertex(opp_v);
    opp_h->set_vertex(v);
    h->set_direction(opp_h->direction());

    // Update the vertices accordingly.
    v->set_halfedge(opp_h);
    opp_v->set_halfedge(h);
  }

  // Reverse the ccb chains (inner and outer) of each face.
  for (DFace_iterator fit = dcel.faces_begin(); fit != dcel.faces_end(); ++fit) {
    for (DOuter_ccb_iterator out_ccb_it = fit->outer_ccbs_begin();
         out_ccb_it != fit->outer_ccbs_end(); ++out_ccb_it)
    {
      reverse_ccb_chain(*out_ccb_it);
    }
    for (DInner_ccb_iterator in_ccb_it = fit->inner_ccbs_begin();
         in_ccb_it != fit->inner_ccbs_end(); ++in_ccb_it)
    {
      reverse_ccb_chain(*in_ccb_it);
    }
  }

  // Update the spherical and south face.
  if (spherical_face != south_face) {
    // The spherical face does not contain the north pole after the reflection.
    // Swap it with the south face.

    // Convert the spherical face first inner ccb to outer ccb.
    inner_ccb_to_outer_ccb(dcel, (*spherical_face->inner_ccbs_begin())->inner_ccb());

    // Convert the south face outer ccbs to inner ccbs.
    DOuter_ccb_iterator occb = south_face->outer_ccbs_begin();
    for (size_t i = south_face->number_of_outer_ccbs(); i > 0; --i) {
      DOuter_ccb* outer_ccb = (*occb)->outer_ccb();
      ++occb;
      outer_ccb_to_inner_ccb(dcel, outer_ccb);
    }
  }

  // Update the dcel.
  accessor.dcel_updated();
}

/*! Reverse a ccb chain, represented by ccb_halfedge. */
template <typename DHalfedge>
void reverse_ccb_chain(DHalfedge* ccb_halfedge)
{
  DHalfedge* ccb_curr_halfedge = ccb_halfedge;

  // This is the first prev link to be overwritten.
  DHalfedge* ccb_last_halfedge_prev = ccb_halfedge->prev()->prev();

  DHalfedge* ccb_next_halfedge = ccb_curr_halfedge->next();
  while (ccb_next_halfedge != ccb_halfedge) {
    // Fix the links of the current halfedge.
    ccb_curr_halfedge->set_next(ccb_curr_halfedge->prev());

    // Advance to the next halfedge.
    ccb_curr_halfedge = ccb_next_halfedge;
    ccb_next_halfedge = ccb_curr_halfedge->next();
  }

  // Fix the last halfedge (first's prev).
  ccb_curr_halfedge->set_next(ccb_last_halfedge_prev);
}

/*! Convert the given outer ccb into an inner ccb in the dcel.
 * This includes creating a new inner ccb and deleting the given outer ccb.
 * \param dcel The dcel in which the conversion is done.
 * \param outer_ccb The outer ccb to be converted.
 */
template <typename Dcel>
void outer_ccb_to_inner_ccb(Dcel& dcel, typename Dcel::Outer_ccb* outer_ccb)
{
  typedef typename Dcel::Halfedge  DHalfedge;
  typedef typename Dcel::Face      DFace;
  typedef typename Dcel::Inner_ccb DInner_ccb;

  DInner_ccb* inner_ccb = dcel.new_inner_ccb();

  DFace* face = outer_ccb->face();
  DHalfedge* first = outer_ccb->halfedge();
  DHalfedge* curr = first;

  // Traverse the outer ccb and assoicate its halfedges with the inner ccb.
  do {
    curr->set_inner_ccb(inner_ccb);
    curr = curr->next();
  } while (curr != first);
  inner_ccb->set_face(face);

  // Update the incident face.
  face->add_inner_ccb(inner_ccb, first);
  face->erase_outer_ccb(outer_ccb);

  dcel.delete_outer_ccb(outer_ccb);
}

/*! Convert the given inner ccb into an outer ccb in the dcel.
 * This includes creating a new outer ccb and deleting the given inner ccb.
 * \param dcel The dcel in which the conversion is done.
 * \param inner_ccb The inner ccb to be converted.
 */
template <typename Dcel>
void inner_ccb_to_outer_ccb(Dcel& dcel, typename Dcel::Inner_ccb* inner_ccb)
{
  typedef typename Dcel::Halfedge       DHalfedge;
  typedef typename Dcel::Face           DFace;
  typedef typename Dcel::Outer_ccb      DOuter_ccb;

  DOuter_ccb* outer_ccb = dcel.new_outer_ccb();

  DFace* face = inner_ccb->face();
  DHalfedge* first = inner_ccb->halfedge();
  DHalfedge* curr = first;

  // Traverse the inner ccb and assoicate its halfedges with the outer ccb.
  do {
    curr->set_outer_ccb(outer_ccb);
    curr = curr->next();
  } while (curr != first);
  outer_ccb->set_face(face);

  // Update the incident face.
  face->add_outer_ccb(outer_ccb, first);
  face->erase_inner_ccb(inner_ccb);

  dcel.delete_inner_ccb(inner_ccb);
}

/*! The following overload takes place if GeomeTraitsRes support reflection.
 * It reflects the arrangement using the traits' Reflect_2 functor.
 */
template <typename GeomeTraits, typename GeomeTraitsRes,
          typename TopolTraits, typename TopolTraitsRes>
typename boost::enable_if_c<has_Reflect_2<GeomeTraitsRes>::value, void>::type
reflect_arrangement(const Arrangement_on_surface_2<GeomeTraits, TopolTraits>& arr,
                    Arrangement_on_surface_2<GeomeTraitsRes, TopolTraitsRes>& arr_res,
                    bool should_merge)
{
  // Copy the initial arrangement and reflect it in-place

  typedef Arrangement_on_surface_2<GeomeTraitsRes, TopolTraitsRes>
                                                        Arr_res;
  typedef CGAL::Arr_accessor<Arr_res>                   Arr_accessor;

  // Arrangement typedefs
  typedef typename Arr_res::X_monotone_curve_2          X_monotone_curve_2;
  typedef typename Arr_res::Vertex_iterator             Vertex_iterator;
  typedef typename Arr_res::Edge_iterator               Edge_iterator;
  typedef typename Arr_res::Halfedge_handle             Halfedge_handle;
  typedef typename Arr_res::Has_identified_sides_category
    Has_identified_sides_category;

  // Geometry traits typedefs
  typedef typename GeomeTraitsRes::Reflect_2            Reflect_2;
  typedef typename GeomeTraitsRes::Are_mergeable_2      Are_mergeable_2;
  typedef typename GeomeTraitsRes::Merge_2              Merge_2;
  typedef typename GeomeTraitsRes::Equal_2              Equal_2;

  // Some type assertions (not all, but better then nothing).
  CGAL_static_assertion
  ((boost::is_convertible<typename GeomeTraits::Point_2,
                          typename GeomeTraitsRes::Point_2>::value));
  CGAL_static_assertion
  ((boost::is_convertible<typename GeomeTraits::X_monotone_curve_2,
                          typename GeomeTraitsRes::X_monotone_curve_2>::value));

  // The result arrangement cannot be the input arrangement.
  CGAL_precondition((void *)(&arr_res) != (void *)(&arr));

  // Copy the initial arrangement.
  arr_res.assign(arr);

  // TODO: proper dispatching based on LR identified and BT contracted tags.
  reflect_arrangement_impl(arr_res, should_merge,
                           Has_identified_sides_category());
}

/*! The following overload takes place if GeomeTraitsRes do not support
 * reflection.
 * Reports an appropriate error message.
 */
template <typename GeomeTraits, typename GeomeTraitsRes,
          typename TopolTraits, typename TopolTraitsRes>
typename boost::enable_if_c<!has_Reflect_2<GeomeTraitsRes>::value, void>::type
reflect_arrangement(const Arrangement_on_surface_2<GeomeTraits, TopolTraits>& arr,
                    Arrangement_on_surface_2<GeomeTraitsRes, TopolTraitsRes>& arr_res,
                    bool should_merge)
{
  CGAL_error_msg("The result arrangement traits does not support reflection!");
}

/*! Computes the reflection of an arrangement through the origin.
 * \param arr The arrangement to reflect.
 * \param arr_res Output: The resulting arrangement.
 * \param should_merge Should merge curves previously divided by boundary.
 */
template <typename GeomeTraits, typename GeomeTraitsRes,
          typename TopolTraits, typename TopolTraitsRes>
void reflect(const Arrangement_on_surface_2<GeomeTraits, TopolTraits>& arr,
             Arrangement_on_surface_2<GeomeTraitsRes, TopolTraitsRes>& arr_res,
             bool should_merge = true)
{
  reflect_arrangement(arr, arr_res, should_merge);
}

} // namespace CGAL

#endif
