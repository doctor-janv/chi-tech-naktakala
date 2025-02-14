#ifndef SPATIAL_DISCRETIZATION_H
#define SPATIAL_DISCRETIZATION_H

#include "mesh/chi_mesh.h"
#include "math/Quadratures/quadrature.h"
#include "math/chi_math.h"
#include "math/UnknownManager/unknown_manager.h"
#include "mesh/Cell/cell.h"
#include "math/SpatialDiscretization/CellMappings/cell_mapping_base.h"

#include <petscksp.h>

#include <vector>
#include <map>

namespace chi_math
{
class SpatialDiscretization
{
public:
  const UnknownManager UNITARY_UNKNOWN_MANAGER;

protected:
  const chi_mesh::MeshContinuum& ref_grid_;
  std::vector<std::unique_ptr<CellMapping>> cell_mappings_;
  std::map<uint64_t, std::shared_ptr<CellMapping>> nb_cell_mappings_;

  uint64_t local_block_address_ = 0;
  std::vector<uint64_t> locJ_block_address_;
  std::vector<uint64_t> locJ_block_size_;

  uint64_t local_base_block_size_ = 0;
  uint64_t globl_base_block_size_ = 0;

private:
  const SpatialDiscretizationType type_;

protected:
  const CoordinateSystemType coord_sys_type_;
  typedef SpatialDiscretizationType SDMType;
  // 00
  explicit SpatialDiscretization(const chi_mesh::MeshContinuum& in_grid,
                                 CoordinateSystemType in_cs_type,
                                 SDMType in_type = SDMType::UNDEFINED)
    : UNITARY_UNKNOWN_MANAGER(
        {std::make_pair(chi_math::UnknownType::SCALAR, 0)}),
      ref_grid_(in_grid),
      type_(in_type),
      coord_sys_type_(in_cs_type)
  {
  }

  // 01 AddViewOfContinuum
public:
  const CellMapping& GetCellMapping(const chi_mesh::Cell& cell) const;
  SpatialDiscretizationType Type() const;
  /**Returns the reference grid on which this discretization is based.*/
  const chi_mesh::MeshContinuum& Grid() const;
  CoordinateSystemType GetCoordinateSystemType() const;

  // 02 OrderNodes

public:
  // 03
  virtual void
  BuildSparsityPattern(std::vector<int64_t>& nodal_nnz_in_diag,
                       std::vector<int64_t>& nodal_nnz_off_diag,
                       const UnknownManager& unknown_manager) const = 0;

  // 04 Mappings
  virtual int64_t MapDOF(const chi_mesh::Cell& cell,
                         unsigned int node,
                         const UnknownManager& unknown_manager,
                         unsigned int unknown_id,
                         unsigned int component) const = 0;

  virtual int64_t MapDOFLocal(const chi_mesh::Cell& cell,
                              unsigned int node,
                              const UnknownManager& unknown_manager,
                              unsigned int unknown_id,
                              unsigned int component) const = 0;

  virtual int64_t MapDOF(const chi_mesh::Cell& cell,
                         unsigned int node) const = 0;
  virtual int64_t MapDOFLocal(const chi_mesh::Cell& cell,
                              unsigned int node) const = 0;

  // 05 Utils
  virtual size_t
  GetNumLocalDOFs(const UnknownManager& unknown_manager) const = 0;
  virtual size_t
  GetNumGlobalDOFs(const UnknownManager& unknown_manager) const = 0;

  virtual size_t
  GetNumGhostDOFs(const UnknownManager& unknown_manager) const = 0;
  virtual std::vector<int64_t>
  GetGhostDOFIndices(const UnknownManager& unknown_manager) const = 0;

  size_t GetNumLocalAndGhostDOFs(const UnknownManager& unknown_manager) const;

  virtual size_t GetCellNumNodes(const chi_mesh::Cell& cell) const = 0;

  virtual std::vector<chi_mesh::Vector3>
  GetCellNodeLocations(const chi_mesh::Cell& cell) const = 0;

  std::vector<std::vector<std::vector<int>>>
  MakeInternalFaceNodeMappings(double tolerance = 1.0e-12) const;

  void CopyVectorWithUnknownScope(const std::vector<double>& from_vector,
                                  std::vector<double>& to_vector,
                                  const UnknownManager& from_vec_uk_structure,
                                  unsigned int from_vec_uk_id,
                                  const UnknownManager& to_vec_uk_structure,
                                  unsigned int to_vec_uk_id) const;

public:
  /**Develops a localized view of a petsc vector.
   * Each spatial discretization can have a specialization of this
   * method.*/
  virtual void LocalizePETScVector(Vec petsc_vector,
                                   std::vector<double>& local_vector,
                                   const UnknownManager& unknown_manager) const;
  /**Develops a localized view of a petsc vector.
   * Each spatial discretization can have a specialization of this
   * method.*/
  virtual void
  LocalizePETScVectorWithGhosts(Vec petsc_vector,
                                std::vector<double>& local_vector,
                                const UnknownManager& unknown_manager) const;
};
} // namespace chi_math

#endif