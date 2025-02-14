#include "lbs_sweepchunk.h"

#include "math/SpatialDiscretization/spatial_discretization.h"

#define scint static_cast<int>

namespace lbs
{

void LBSSweepChunk::Sweep(chi_mesh::sweep_management::AngleSet* angle_set)
{
  const SubSetInfo& grp_ss_info =
    groupset_.grp_subset_infos_[angle_set->ref_subset];

  gs_ss_size_ = grp_ss_info.ss_size;
  gs_ss_begin_ = grp_ss_info.ss_begin;
  gs_gi_ = groupset_.groups_[gs_ss_begin_].id_;

  int deploc_face_counter = -1;
  int preloc_face_counter = -1;

  sweep_surface_status_info_.angle_set = angle_set;
  sweep_surface_status_info_.fluds = &(*angle_set->fluds);
  sweep_surface_status_info_.surface_source_active = IsSurfaceSourceActive();
  sweep_surface_status_info_.gs_ss_size_ = gs_ss_size_;
  sweep_surface_status_info_.gs_ss_begin_ = gs_ss_begin_;
  sweep_surface_status_info_.gs_gi_ = gs_gi_;

  // ====================================================== Loop over each
  //                                                        cell
  const auto& spds = angle_set->GetSPDS();
  const auto& spls = spds.spls.item_id;
  const size_t num_spls = spls.size();
  for (size_t spls_index = 0; spls_index < num_spls; ++spls_index)
  {
    cell_local_id_ = spls[spls_index];
    cell_ = &grid_.local_cells[cell_local_id_];
    cell_mapping_ = &grid_fe_view_.GetCellMapping(*cell_);
    cell_transport_view_ = &grid_transport_view_[cell_->local_id_];

    using namespace chi_mesh::sweep_management;
    const auto& face_orientations =
      spds.cell_face_orientations_[cell_local_id_];

    cell_num_faces_ = cell_->faces_.size();
    cell_num_nodes_ = cell_mapping_->NumNodes();
    const auto& sigma_t = xs_.at(cell_->material_id_)->SigmaTotal();

    sweep_surface_status_info_.spls_index = spls_index;
    sweep_surface_status_info_.cell_local_id = cell_local_id_;

    // =============================================== Get Cell matrices
    const auto& fe_intgrl_values = unit_cell_matrices_[cell_local_id_];
    G_ = &fe_intgrl_values.G_matrix;
    M_ = &fe_intgrl_values.M_matrix;
    M_surf_ = &fe_intgrl_values.face_M_matrices;
    IntS_shapeI_ = &fe_intgrl_values.face_Si_vectors;

    for (auto& callback : cell_data_callbacks_)
      callback();

    // =============================================== Loop over angles in set
    const int ni_deploc_face_counter = deploc_face_counter;
    const int ni_preloc_face_counter = preloc_face_counter;
    const size_t as_num_angles = angle_set->angles.size();
    for (size_t angle_set_index = 0; angle_set_index < as_num_angles;
         ++angle_set_index)
    {
      direction_num_ = angle_set->angles[angle_set_index];
      omega_ = groupset_.quadrature_->omegas_[direction_num_];
      direction_qweight_ = groupset_.quadrature_->weights_[direction_num_];

      sweep_surface_status_info_.angle_set_index = angle_set_index;
      sweep_surface_status_info_.angle_num = direction_num_;

      deploc_face_counter = ni_deploc_face_counter;
      preloc_face_counter = ni_preloc_face_counter;

      // ======================================== Reset right-handside
      for (int gsg = 0; gsg < gs_ss_size_; ++gsg)
        b_[gsg].assign(cell_num_nodes_, 0.0);

      ExecuteKernels(direction_data_callbacks_and_kernels_);

      // ======================================== Upwinding structure
      sweep_surface_status_info_.in_face_counter = 0;
      sweep_surface_status_info_.preloc_face_counter = 0;
      sweep_surface_status_info_.out_face_counter = 0;
      sweep_surface_status_info_.deploc_face_counter = 0;

      // ======================================== Update face orientations
      face_mu_values_.assign(cell_num_faces_, 0.0);
      for (int f = 0; f < cell_num_faces_; ++f)
      {
        const double mu = omega_.Dot(cell_->faces_[f].normal_);
        face_mu_values_[f] = mu;
      }

      // ======================================== Surface integrals
      int in_face_counter = -1;
      for (int f = 0; f < cell_num_faces_; ++f)
      {
        const auto& face = cell_->faces_[f];

        if (face_orientations[f] != FaceOrientation::INCOMING) continue;

        const bool local = cell_transport_view_->IsFaceLocal(f);
        const bool boundary = not face.has_neighbor_;
        const uint64_t bndry_id = face.neighbor_id_;

        if (local) ++in_face_counter;
        else if (not boundary)
          ++preloc_face_counter;

        sweep_surface_status_info_.in_face_counter = in_face_counter;
        sweep_surface_status_info_.preloc_face_counter = preloc_face_counter;
        sweep_surface_status_info_.bndry_id = bndry_id;
        sweep_surface_status_info_.f = f;
        sweep_surface_status_info_.on_local_face = local;
        sweep_surface_status_info_.on_boundary = boundary;

        // IntSf_mu_psi_Mij_dA
        ExecuteKernels(surface_integral_kernels_);
      } // for f

      // ======================================== Looping over groups,
      //                                          Assembling mass terms
      for (int gsg = 0; gsg < gs_ss_size_; ++gsg)
      {
        g_ = gs_gi_ + gsg;
        gsg_ = gsg;
        sigma_tg_ = sigma_t[g_];

        ExecuteKernels(mass_term_kernels_);

        // ================================= Solve system
        chi_math::GaussElimination(Atemp_, b_[gsg], scint(cell_num_nodes_));
      }

      // ======================================== Flux updates
      ExecuteKernels(flux_update_kernels_);

      // ======================================== Perform outgoing
      //                                               surface operations
      int out_face_counter = -1;
      for (int f = 0; f < cell_num_faces_; ++f)
      {
        if (face_orientations[f] == FaceOrientation::INCOMING) continue;

        // ================================= Set flags and counters
        out_face_counter++;
        const auto& face = cell_->faces_[f];
        const bool local = cell_transport_view_->IsFaceLocal(f);
        const bool boundary = not face.has_neighbor_;
        const uint64_t bndry_id = face.neighbor_id_;

        bool reflecting_bndry = false;
        if (boundary)
          if (angle_set->ref_boundaries[bndry_id]->IsReflecting())
            reflecting_bndry = true;

        if (not boundary and not local) ++deploc_face_counter;

        sweep_surface_status_info_.out_face_counter = out_face_counter;
        sweep_surface_status_info_.deploc_face_counter = deploc_face_counter;
        sweep_surface_status_info_.bndry_id = bndry_id;
        sweep_surface_status_info_.f = f;
        sweep_surface_status_info_.on_local_face = local;
        sweep_surface_status_info_.on_boundary = boundary;
        sweep_surface_status_info_.is_reflecting_bndry_ = reflecting_bndry;

        OutgoingSurfaceOperations();
      } // for face

      ExecuteKernels(post_cell_dir_sweep_callbacks_);
    } // for n
  }   // for cell
}

} // namespace lbs