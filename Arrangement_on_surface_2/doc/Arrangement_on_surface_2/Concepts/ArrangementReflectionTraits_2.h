
/*!
\ingroup PkgArrangement2ConceptsTraits
\cgalConcept

The concept `ArrangementReflectionTraits_2` refines the basic arrangement-traits concept.
A model of this concept is able to perform a reflection of the arrangement's geometric objects.

\cgalRefines `ArrangementBasicTraits_2`

\cgalHasModel `CGAL::Arr_segment_traits_2<Kernel>`

\sa `reflect` 

*/

class ArrangementReflectionTraits_2 {
public:

/// @} 

/// \name Functor Types 
/// @{

typedef unspecified_type Reflect_2;

/// @} 

/// \name Accessing Functor Objects 
/// @{

Reflect_2 reflect_2_object() const;

/// @} 

}; /* end ArrangementReflectionTraits_2 */

