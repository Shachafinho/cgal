namespace CGAL {

/*! \ingroup PkgArrangement2Funcs
 *
 * \brief Computes the reflection of an arrangement through the origin.
 * \details Computes the reflection of an arrangement through the origin.
 */
template <typename GeomeTraits, typename GeomeTraitsRes,
          typename TopolTraits, typename TopolTraitsRes>
void reflect(const Arrangement_on_surface_2<GeomeTraits, TopolTraits>& arr,
             Arrangement_on_surface_2<GeomeTraitsRes, TopolTraitsRes>& arr_res);

} // namesapce CGAL
