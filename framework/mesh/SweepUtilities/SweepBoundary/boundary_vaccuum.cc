#include "sweep_boundaries.h"

//###################################################################
/**Returns a pointer to a heterogeneous flux storage location.*/
double* chi_mesh::sweep_management::BoundaryVaccuum::
  HeterogeneousPsiIncoming(uint64_t cell_local_id,
                           int face_num,
                           int fi,
                           int angle_num,
                           int group_num,
                           int gs_ss_begin)
{
  return &boundary_flux_[group_num];
}