#include "FLUDS.h"
#include "mesh/SweepUtilities/SPDS/SPDS.h"

#include "mesh/Cell/cell.h"

#include "chi_runtime.h"
#include "chi_log.h"

typedef std::vector<std::pair<int,short>> LockBox;

//###################################################################
/**Performs Incident mapping for Polyhedron cell.*/
void chi_mesh::sweep_management::PRIMARY_FLUDS::
LocalIncidentMapping(const chi_mesh::Cell& cell,
                     const SPDS& spds,
                     std::vector<int>&  local_so_cell_mapping)
{
  chi_mesh::MeshContinuumPtr grid = spds.grid;
  auto& cell_nodal_mapping = grid_nodal_mappings[cell.local_id_];
  std::vector<std::pair<int,std::vector<short>>> inco_face_dof_mapping;

  short        incoming_face_count=-1;

  //=================================================== Loop over faces
  //           INCIDENT                                 but process
  //                                                    only incident faces
  for (short f=0; f < cell.faces_.size(); f++)
  {
    const CellFace& face = cell.faces_[f];
    const auto& orienation = spds.cell_face_orientations_[cell.local_id_][f];

    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Incident face
    if (orienation == FaceOrientation::INCOMING)
    {
      if (face.IsNeighborLocal(*grid))
      {
        incoming_face_count++;
        //======================================== Find associated face for
        //                                         dof mapping
        int ass_face = cell_nodal_mapping[f].associated_face;

        std::pair<int,std::vector<short>> dof_mapping;
        dof_mapping.second = cell_nodal_mapping[f].node_mapping;

        //======================================== Find associated face
        //                                         counter for slot lookup
        const auto& adj_cell = grid->cells[face.neighbor_id_];
        const int adj_so_index = local_so_cell_mapping[adj_cell.local_id_];
        const auto& face_oris = spds.cell_face_orientations_[adj_cell.local_id_];
        int ass_f_counter = -1;

        int out_f = -1;
        for (size_t af=0; af < adj_cell.faces_.size(); ++af)
        {
          if (face_oris[af] == FaceOrientation::OUTGOING) {++out_f;}

          if (af == ass_face)
          {
            ass_f_counter = out_f;
            break;
          }
        }

        dof_mapping.first = /*local_psi_stride*G**/
          so_cell_outb_face_slot_indices[adj_so_index][ass_f_counter];

        dof_mapping.second.shrink_to_fit();
        inco_face_dof_mapping.push_back(dof_mapping);
      }//if local
    }//if incident
  }//for incindent f

  auto inco_face_info_array = new INCOMING_FACE_INFO[inco_face_dof_mapping.size()];
  for (int i=0; i<inco_face_dof_mapping.size(); ++i)
    inco_face_info_array[i].Setup(inco_face_dof_mapping[i]);

  so_cell_inco_face_dof_indices.push_back(inco_face_info_array);


}