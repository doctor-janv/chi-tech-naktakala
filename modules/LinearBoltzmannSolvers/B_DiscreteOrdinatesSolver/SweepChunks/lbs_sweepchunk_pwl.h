#ifndef LBS_SWEEPCHUNK_PWL_H
#define LBS_SWEEPCHUNK_PWL_H

#include "mesh/MeshContinuum/chi_meshcontinuum.h"

#include "math/SpatialDiscretization/spatial_discretization.h"

#include "LinearBoltzmannSolvers/A_LBSSolver/Groupset/lbs_groupset.h"

#include "mesh/SweepUtilities/sweepchunk_base.h"


namespace lbs
{
//###################################################################
/**Sweep chunk for cartesian PWLD discretization.*/
class SweepChunkPWL : public chi_mesh::sweep_management::SweepChunk
{
protected:
  const std::shared_ptr<chi_mesh::MeshContinuum> grid_view_;
  const chi_math::SpatialDiscretization& grid_fe_view_;
  const std::vector<UnitCellMatrices>& unit_cell_matrices_;
  std::vector<lbs::CellLBSView>& grid_transport_view_;
  const std::vector<double>& q_moments_;
  LBSGroupset& groupset_;
  const std::map<int, XSPtr>& xs_;
  const int num_moments_;
  const size_t num_groups_;
  const int max_num_cell_dofs_;
  const bool save_angular_flux_;

  //Runtime params
  bool a_and_b_initialized_;
  std::vector<std::vector<double>> Amat_;
  std::vector<std::vector<double>> Atemp_;
  std::vector<double> source_;
  std::vector<std::vector<double>> b_;

public:

  SweepChunkPWL(std::shared_ptr<chi_mesh::MeshContinuum> grid_ptr,
                const chi_math::SpatialDiscretization& discretization,
                const std::vector<UnitCellMatrices>& unit_cell_matrices,
                std::vector<lbs::CellLBSView>& cell_transport_views,
                std::vector<double>& destination_phi,
                std::vector<double>& destination_psi,
                const std::vector<double>& source_moments,
                LBSGroupset& groupset,
                const std::map<int, XSPtr>& xs,
                int num_moments,
                int max_num_cell_dofs);

  void Sweep(chi_mesh::sweep_management::AngleSet* angle_set) override;


  struct Upwinder
  {
    chi_mesh::sweep_management::FLUDS& fluds;
    chi_mesh::sweep_management::AngleSet* angle_set;
    size_t spls_index;
    size_t angle_set_index;
    int in_face_counter;
    int preloc_face_counter;
    int out_face_counter;
    int deploc_face_counter;
    uint64_t bndry_id;
    int angle_num;
    uint64_t cell_local_id;
    int f;
    int gs_gi;
    size_t gs_ss_begin;
    bool surface_source_active;

    const double* GetUpwindPsi(int fj, bool local, bool boundary) const;
    double* GetDownwindPsi(int fi,
                           bool local,
                           bool boundary,
                           bool reflecting_bndry) const;
  };

};
}


#endif
