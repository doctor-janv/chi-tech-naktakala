#include "sweepscheduler.h"

#include "chi_runtime.h"
#include "chi_mpi.h"
#include "chi_log.h"

#include <sstream>
#include <algorithm>

//###################################################################
/**Initializes the Depth-Of-Graph algorithm.*/
void chi_mesh::sweep_management::SweepScheduler::InitializeAlgoDOG()
{
  //================================================== Load all anglesets
  //                                                   in preperation for
  //                                                   sorting
  //======================================== Loop over angleset groups
  for (size_t q=0; q<angle_agg.angle_set_groups.size(); q++)
  {
    TAngleSetGroup& angleset_group = angle_agg.angle_set_groups[q];

    //================================= Loop over anglesets in group
    size_t num_anglesets = angleset_group.angle_sets.size();
    for (size_t as=0; as<num_anglesets; as++)
    {
      auto angleset                 = angleset_group.angle_sets[as];
      const auto& spds              = angleset->GetSPDS();
      const TLEVELED_GRAPH& leveled_graph = spds.global_sweep_planes;

      //========================== Find location depth
      int loc_depth = -1;
      for (size_t level=0; level<leveled_graph.size(); level++)
      {
        for (size_t index=0; index<leveled_graph[level].item_id.size(); index++)
        {
          if (leveled_graph[level].item_id[index] == Chi::mpi.location_id)
          {
            loc_depth = static_cast<int>(leveled_graph.size()-level);
            break;
          }
        }//for locations in plane
      }//for sweep planes

      //========================== Set up rule values
      if (loc_depth>=0)
      {
        RULE_VALUES new_rule_vals(angleset);
        new_rule_vals.depth_of_graph = loc_depth;
        new_rule_vals.set_index      = as + q * num_anglesets;

        new_rule_vals.sign_of_omegax = (spds.omega.x >= 0)?2:1;
        new_rule_vals.sign_of_omegay = (spds.omega.y >= 0)?2:1;
        new_rule_vals.sign_of_omegaz = (spds.omega.z >= 0)?2:1;

        rule_values.push_back(new_rule_vals);
      }
      else
      {
        Chi::log.LogAllError()
          << "Location depth not found in Depth-Of-Graph algorithm.";
        Chi::Exit(EXIT_FAILURE);
      }

    }//for anglesets
  }//for quadrants/anglesetgroups

  //================================================== Init sort functions
  struct
  {
    bool operator()(const RULE_VALUES& a, const RULE_VALUES& b)
    {
      return a.depth_of_graph > b.depth_of_graph;
    }
  }compare_D;

  struct
  {
    bool operator()(const RULE_VALUES& a, const RULE_VALUES& b)
    {
      return (a.depth_of_graph == b.depth_of_graph) and
             (a.sign_of_omegax > b.sign_of_omegax);
    }
  }compare_omega_x;

  struct
  {
    bool operator()(const RULE_VALUES& a, const RULE_VALUES& b)
    {
      return (a.depth_of_graph == b.depth_of_graph) and
             (a.sign_of_omegax == b.sign_of_omegax) and
             (a.sign_of_omegay > b.sign_of_omegay);
    }
  }compare_omega_y;

  struct
  {
    bool operator()(const RULE_VALUES& a, const RULE_VALUES& b)
    {
      return (a.depth_of_graph == b.depth_of_graph) and
             (a.sign_of_omegax == b.sign_of_omegax) and
             (a.sign_of_omegay == b.sign_of_omegay) and
             (a.sign_of_omegaz > b.sign_of_omegaz);
    }
  }compare_omega_z;

  //================================================== Sort
  std::stable_sort(rule_values.begin(),rule_values.end(),compare_D);
  std::stable_sort(rule_values.begin(),rule_values.end(),compare_omega_x);
  std::stable_sort(rule_values.begin(),rule_values.end(),compare_omega_y);
  std::stable_sort(rule_values.begin(),rule_values.end(),compare_omega_z);

}

//###################################################################
/**Executes the Depth-Of-Graph algorithm.*/
void chi_mesh::sweep_management::SweepScheduler::
  ScheduleAlgoDOG(SweepChunk& sweep_chunk)
{
  typedef ExecutionPermission ExePerm;
  typedef AngleSetStatus Status;

  Chi::log.LogEvent(sweep_event_tag, chi::ChiLog::EventType::EVENT_BEGIN);

  auto ev_info =
    std::make_shared<chi::ChiLog::EventInfo>(std::string("Sweep initiated"));

  Chi::log.LogEvent(sweep_event_tag, chi::ChiLog::EventType::SINGLE_OCCURRENCE, ev_info);

  //==================================================== Loop till done
  bool finished = false;
  size_t scheduled_angleset = 0;
  while (!finished)
  {
    finished = true;
    for (auto & rule_value : rule_values)
    {
      auto angleset = rule_value.angle_set;
      const int angset_number = static_cast<int>(rule_value.set_index);

      //=============================== Query angleset status
      // Status will here be one of the following:
      //  - RECEIVING.
      //      Meaning it is either waiting for messages or actively receiving it
      //  - READY_TO_EXECUTE.
      //      Meaning it has received all upstream data and can be executed
      //  - FINISHED.
      //      Meaning the angleset has executed its sweep chunk
      Status status = angleset->
        AngleSetAdvance(sweep_chunk,
                        angset_number,
                        sweep_timing_events_tag,
                        ExePerm::NO_EXEC_IF_READY);

      //=============================== Execute if ready and allowed
      // If this angleset is the one scheduled to run
      // and it is ready then it will be given permission
      if (status == Status::READY_TO_EXECUTE)
      {
        std::stringstream message_i;
        message_i
          << "Angleset " << angset_number
          << " executed on location " << Chi::mpi.location_id;

        auto ev_info_i = std::make_shared<chi::ChiLog::EventInfo>(message_i.str());

        Chi::log.LogEvent(sweep_event_tag,
                          chi::ChiLog::EventType::SINGLE_OCCURRENCE, ev_info_i);

        status = angleset->
          AngleSetAdvance(sweep_chunk,
                          angset_number,
                          sweep_timing_events_tag,
                          ExePerm::EXECUTE);

        std::stringstream message_f;
        message_f
          << "Angleset " << angset_number
          << " finished on location " << Chi::mpi.location_id;

        auto ev_info_f = std::make_shared<chi::ChiLog::EventInfo>(message_f.str());

        Chi::log.LogEvent(sweep_event_tag,
                          chi::ChiLog::EventType::SINGLE_OCCURRENCE, ev_info_f);

        scheduled_angleset++; //Schedule the next angleset
      }

      if (status != Status::FINISHED)
        finished = false;
    }//for each angleset rule
  }//while not finished

  //================================================== Receive delayed data
  Chi::mpi.Barrier();
  bool received_delayed_data = false;
  while (not received_delayed_data)
  {
    received_delayed_data = true;
    for (auto& sorted_angleset : rule_values)
    {
      auto& as = sorted_angleset.angle_set;

      if (as->FlushSendBuffers() == Status::MESSAGES_PENDING)
        received_delayed_data = false;

      if (not as->ReceiveDelayedData(sorted_angleset.set_index))
        received_delayed_data = false;
    }
  }

  //================================================== Reset all
  for (auto& angset_group : angle_agg.angle_set_groups)
    angset_group.ResetSweep();

  for (auto& [bid, bndry] : angle_agg.sim_boundaries)
  {
    if (bndry->Type() == chi_mesh::sweep_management::BoundaryType::REFLECTING)
    {
      auto rbndry = std::static_pointer_cast<
        chi_mesh::sweep_management::BoundaryReflecting>(bndry);
      rbndry->ResetAnglesReadyStatus();
    }
  }

  Chi::log.LogEvent(sweep_event_tag, chi::ChiLog::EventType::EVENT_END);
}