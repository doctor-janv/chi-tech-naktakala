-- 3D Transport test with Vacuum BCs.
-- SDM: PWLD
-- Test: Max-value1=6.55387e+00
--       Max-value2=1.02940e+00

num_procs = 4





--############################################### Check num_procs
if (check_num_procs==nil and chi_number_of_processes ~= num_procs) then
  chiLog(LOG_0ERROR,"Incorrect amount of processors. " ..
    "Expected "..tostring(num_procs)..
    ". Pass check_num_procs=false to override if possible.")
  os.exit(false)
end

--############################################### Setup mesh
chiMeshHandlerCreate()

umesh = chiUnpartitionedMeshFromEnsightGold("../../../../resources/TestMeshes/Sphere.case")

chiSurfaceMesherCreate(SURFACEMESHER_PREDEFINED)
chiVolumeMesherCreate(VOLUMEMESHER_UNPARTITIONED, umesh)

chiVolumeMesherSetProperty(VOLUMEPARTITION_X,2)
chiVolumeMesherSetProperty(VOLUMEPARTITION_Y,2)
chiVolumeMesherSetProperty(CUTS_X,0.0)
chiVolumeMesherSetProperty(CUTS_Y,0.0)

chiSurfaceMesherExecute()
chiVolumeMesherExecute()

--############################################### Add materials
materials = {}
materials[1] = chiPhysicsAddMaterial("Test Material");
materials[2] = chiPhysicsAddMaterial("Test Material2");

chiPhysicsMaterialAddProperty(materials[1],TRANSPORT_XSECTIONS)
chiPhysicsMaterialAddProperty(materials[2],TRANSPORT_XSECTIONS)

chiPhysicsMaterialAddProperty(materials[1],ISOTROPIC_MG_SOURCE)
chiPhysicsMaterialAddProperty(materials[2],ISOTROPIC_MG_SOURCE)

num_groups = 5
chiPhysicsMaterialSetProperty(materials[1],TRANSPORT_XSECTIONS,
  CHI_XSFILE,"xs_graphite_pure.cxs")
chiPhysicsMaterialSetProperty(materials[2],TRANSPORT_XSECTIONS,
  CHI_XSFILE,"xs_graphite_pure.cxs")

src={}
for g=1,num_groups do
  src[g] = 0.0
end

chiPhysicsMaterialSetProperty(materials[2],ISOTROPIC_MG_SOURCE,FROM_ARRAY,src)
src[1]=1.0
chiPhysicsMaterialSetProperty(materials[1],ISOTROPIC_MG_SOURCE,FROM_ARRAY,src)

--############################################### Setup Physics
pquad0 = chiCreateProductQuadrature(GAUSS_LEGENDRE_CHEBYSHEV,2, 2)

lbs_block =
{
  num_groups = num_groups,
  groupsets =
  {
    {
      groups_from_to = {0, num_groups-1},
      angular_quadrature_handle = pquad0,
      angle_aggregation_type = "single",
      angle_aggregation_num_subsets = 1,
      groupset_num_subsets = 1,
      inner_linear_method = "gmres",
      l_abs_tol = 1.0e-6,
      l_max_its = 300,
      gmres_restart_interval = 100,
    },
  }
}

lbs_options =
{
  scattering_order = 0,
}

phys1 = lbs.DiscreteOrdinatesSolver.Create(lbs_block)
lbs.SetOptions(phys1, lbs_options)

--############################################### Initialize and Execute Solver
ss_solver = lbs.SteadyStateSolver.Create({lbs_solver_handle = phys1})

chiSolverInitialize(ss_solver)
chiSolverExecute(ss_solver)

--############################################### Get field functions
fflist,count = chiLBSGetScalarFieldFunctionList(phys1)

--############################################### Slice plot
--slices = {}
--for k=1,count do
--    slices[k] = chiFFInterpolationCreate(SLICE)
--    chiFFInterpolationSetProperty(slices[k],SLICE_POINT,0.0,0.0,0.8001)
--    chiFFInterpolationSetProperty(slices[k],ADD_FIELDFUNCTION,fflist[k])
--    --chiFFInterpolationSetProperty(slices[k],SLICE_TANGENT,0.393,1.0-0.393,0)
--    --chiFFInterpolationSetProperty(slices[k],SLICE_NORMAL,-(1.0-0.393),-0.393,0.0)
--    --chiFFInterpolationSetProperty(slices[k],SLICE_BINORM,0.0,0.0,1.0)
--    chiFFInterpolationInitialize(slices[k])
--    chiFFInterpolationExecute(slices[k])
--    chiFFInterpolationExportPython(slices[k])
--end

--############################################### Volume integrations
vol0 = chi_mesh.RPPLogicalVolume.Create({infx=true, infy=true, infz=true})
ffi1 = chiFFInterpolationCreate(VOLUME)
curffi = ffi1
chiFFInterpolationSetProperty(curffi,OPERATION,OP_MAX)
chiFFInterpolationSetProperty(curffi,LOGICAL_VOLUME,vol0)
chiFFInterpolationSetProperty(curffi,ADD_FIELDFUNCTION,fflist[1])

chiFFInterpolationInitialize(curffi)
chiFFInterpolationExecute(curffi)
maxval = chiFFInterpolationGetValue(curffi)

chiLog(LOG_0,string.format("Max-value1=%.5e", maxval))

ffi1 = chiFFInterpolationCreate(VOLUME)
curffi = ffi1
chiFFInterpolationSetProperty(curffi,OPERATION,OP_MAX)
chiFFInterpolationSetProperty(curffi,LOGICAL_VOLUME,vol0)
chiFFInterpolationSetProperty(curffi,ADD_FIELDFUNCTION,fflist[2])

chiFFInterpolationInitialize(curffi)
chiFFInterpolationExecute(curffi)
maxval = chiFFInterpolationGetValue(curffi)

chiLog(LOG_0,string.format("Max-value2=%.5e", maxval))

--############################################### Exports
if (master_export == nil) then
  chiExportMultiFieldFunctionToVTK(fflist,"ZPhi3D")
  chiExportFieldFunctionToVTK(fflist[1],"ZPhi3D_g0")
end

--############################################### Plots
if (chi_location_id == 0 and master_export == nil) then
  print("Execution completed")
end


