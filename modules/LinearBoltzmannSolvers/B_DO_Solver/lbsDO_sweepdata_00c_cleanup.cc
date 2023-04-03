#include "lbs_discrete_ordinates_solver.h"

#include "LinearBoltzmannSolvers/A_LBSSolver/Groupset/lbs_groupset.h"

#include "chi_runtime.h"
#include "ChiConsole/chi_console.h"
#include "chi_log.h"
#include "chi_mpi.h"


#include <iomanip>

//###################################################################
/**Clears all the sweep orderings for a groupset in preperation for
 * another.*/
void lbs::LBSDiscreteOrdinatesSolver::ResetSweepOrderings(LBSGroupset& groupset)
{
  chi::log.Log0Verbose1()
    << "Resetting SPDS and FLUDS";

  groupset.angle_agg_->angle_set_groups.clear();

  MPI_Barrier(MPI_COMM_WORLD);

  chi::log.Log()
    << "SPDS and FLUDS reset complete.            Process memory = "
    << std::setprecision(3)
    << chi_objects::ChiConsole::GetMemoryUsageInMB() << " MB";

  double local_app_memory =
    chi::log.ProcessEvent(chi_objects::ChiLog::StdTags::MAX_MEMORY_USAGE,
                          chi_objects::ChiLog::EventOperation::MAX_VALUE);
  double total_app_memory=0.0;
  MPI_Allreduce(&local_app_memory,&total_app_memory,
                1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
  double max_proc_memory=0.0;
  MPI_Allreduce(&local_app_memory,&max_proc_memory,
                1,MPI_DOUBLE,MPI_MAX,MPI_COMM_WORLD);

  chi::log.Log()
    << "\n" << std::setprecision(3)
    << "           Total application memory (max): "
    << total_app_memory/1024.0 << " GB\n"
    << "           Maximum process memory        : "
    << max_proc_memory/1024.0 << " GB\n\n";

}