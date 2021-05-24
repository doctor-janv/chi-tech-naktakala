#include "lbs_linear_boltzmann_solver.h"
#include "IterativeMethods/lbs_iterativemethods.h"

#include "ChiMesh/SweepUtilities/SweepScheduler/sweepscheduler.h"

#include "chi_log.h"
extern ChiLog&     chi_log;

#include "chi_mpi.h"
extern ChiMPI&      chi_mpi;

#include "ChiConsole/chi_console.h"
extern ChiConsole&  chi_console;

#include <iomanip>

//###################################################################
/**Execute the solver.*/
void LinearBoltzmann::Solver::Execute()
{
  MPI_Barrier(MPI_COMM_WORLD);
  int gs=-1;
  for (auto& groupset : group_sets)
  {
    ++gs;
    chi_log.Log(LOG_0)
      << "\n********* Initializing Groupset " << gs << "\n" << std::endl;

    ComputeSweepOrderings(groupset);
    InitFluxDataStructures(groupset);

    InitWGDSA(groupset);
    InitTGDSA(groupset);

    SolveGroupset(groupset, gs);

    CleanUpWGDSA(groupset);
    CleanUpTGDSA(groupset);

    ResetSweepOrderings(groupset);

    MPI_Barrier(MPI_COMM_WORLD);
  }

  chi_log.Log(LOG_0) << "NPTransport solver execution completed\n";
}


//###################################################################
/**Solves a single groupset.*/
void LinearBoltzmann::Solver::SolveGroupset(LBSGroupset& groupset,
                                            int group_set_num)
{
  source_event_tag = chi_log.GetRepeatingEventTag("Set Source");

  //================================================== Setting up required
  //                                                   sweep chunks
  auto sweep_chunk = SetSweepChunk(groupset);
  MainSweepScheduler sweep_scheduler(SchedulingAlgorithm::DEPTH_OF_GRAPH,
                                     groupset.angle_agg,
                                     *sweep_chunk);

  if (groupset.iterative_method == IterativeMethod::CLASSICRICHARDSON)
  {
    ClassicRichardson(groupset, group_set_num, sweep_scheduler,
                      APPLY_MATERIAL_SOURCE |
                      APPLY_AGS_SCATTER_SOURCE | APPLY_WGS_SCATTER_SOURCE |
                      APPLY_FISSION_SOURCE);
  }
  else if (groupset.iterative_method == IterativeMethod::GMRES)
  {
    GMRES(groupset, group_set_num, sweep_scheduler,
          APPLY_SCATTER_SOURCE | APPLY_FISSION_SOURCE,
          APPLY_MATERIAL_SOURCE);
  }

  if (options.write_restart_data)
    WriteRestartData(options.write_restart_folder_name,
                     options.write_restart_file_base);

  chi_log.Log(LOG_0)
    << "Groupset solve complete.                  Process memory = "
    << std::setprecision(3)
    << chi_console.GetMemoryUsageInMB() << " MB";
}

