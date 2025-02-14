#ifndef CHITECH_INPUT_PARAMETERS_H
#define CHITECH_INPUT_PARAMETERS_H

#include "parameter_block.h"
#include "data_types/allowable_range.h"

#include <map>

#define MakeInpParamsForObj(x, y) chi::InputParameters::MakeForObject<x>(y)

namespace chi
{

enum class InputParameterTag
{
  NONE = 0,
  OPTIONAL = 1,
  REQUIRED = 2
};

/**Class for handling input parameters.*/
class InputParameters : public ParameterBlock
{
private:
  /**String to represent class name. If not provided a default will be
   * generated.*/
  std::string class_name_;
  /**Space separated list of group names.*/
  std::string doc_group_;
  std::map<std::string, InputParameterTag> parameter_class_tags_;
  std::map<std::string, std::string> parameter_doc_string_;

  std::map<std::string, std::string> deprecation_warning_tags_;
  std::map<std::string, std::string> deprecation_error_tags_;
  std::map<std::string, std::string> renamed_error_tags_;
  std::map<std::string, bool> type_mismatch_allowed_tags_;
  std::map<std::string, std::string> parameter_link_;

  typedef std::unique_ptr<chi_data_types::AllowableRange> AllowableRangePtr;
  std::map<std::string, AllowableRangePtr> constraint_tags_;

  std::string general_description_;

  /**Parameter names to ignore when trying to assign. For now this
   * "chi_obj_type"*/
  static const std::vector<std::string> system_ignored_param_names_;

  ParameterBlock param_block_at_assignment_;

public:
  InputParameters() = default;
  template <typename T>
  static InputParameters MakeForObject(const ParameterBlock& params)
  {
    auto input_param = T::GetInputParameters();

    input_param.AssignParameters(params);
    return input_param;
  }

public:
  void SetObjectType(const std::string& obj_type);
  std::string ObjectType() const;

  /**Sets the class name to be applied to this object. If not used a
   * default will be generated.*/
  void SetClassName(const std::string& class_name) { class_name_ = class_name; }

  /**Sets a general description of the object that should be included with
   * the object's documentation.*/
  void SetGeneralDescription(const std::string& description)
  {
    general_description_ = description;
  }
  std::string GetGeneralDescription() const { return general_description_; }

  /**Space separated list of doxygen group names to which this documentation
   * should belong.*/
  void SetDocGroup(const std::string& doc_group) { doc_group_ = doc_group; }

  /**Sets a link to the documentation of a different object.*/
  void LinkParameterToBlock(const std::string& param_name,
                            const std::string& block_name);
  /**Gets any linkage information of a parameter.*/
  std::string
  GetParameterDocumentationLink(const std::string& param_name) const;

  std::string GetParameterDocString(const std::string& param_name);

private:
  using ParameterBlock::AddParameter;
  static bool IsParameterIgnored(const std::string& param_name);

public:
  template <typename T>
  void AddOptionalParameter(const std::string& name,
                            T value,
                            const std::string& doc_string)
  {
    AddParameter(name, value);
    parameter_class_tags_[name] = InputParameterTag::OPTIONAL;
    parameter_doc_string_[name] = doc_string;
  }

  void AddOptionalParameterBlock(const std::string& name,
                                 const ParameterBlock& block,
                                 const std::string& doc_string);

  template <typename T>
  void AddOptionalParameterArray(const std::string& name,
                                 const std::vector<T>& array,
                                 const std::string& doc_string)
  {
    AddParameter(name, array);
    parameter_class_tags_[name] = InputParameterTag::OPTIONAL;
    parameter_doc_string_[name] = doc_string;
  }

  void AddOptionalParameterArray(const std::string& name,
                                 const std::vector<ParameterBlock>& array,
                                 const std::string& doc_string);

  template <typename T>
  void AddRequiredParameter(const std::string& name,
                            const std::string& doc_string)
  {
    AddParameter(name, chi_data_types::Varying::DefaultValue<T>());
    parameter_class_tags_[name] = InputParameterTag::REQUIRED;
    parameter_doc_string_[name] = doc_string;
  }

  void AddRequiredParameterBlock(const std::string& name,
                                 const std::string& doc_string);

  void AddRequiredParameterArray(const std::string& name,
                                 const std::string& doc_string);

  template <typename T>
  void ChangeExistingParamToOptional(const std::string& name,
                                     T value,
                                     const std::string& doc_string = "")
  {
    auto& param = GetParam(name);
    param = ParameterBlock(name, value);
    parameter_class_tags_[name] = InputParameterTag::OPTIONAL;
    if (not doc_string.empty()) parameter_doc_string_[name] = doc_string;
  }

  template <typename T>
  void ChangeExistingParamToRequired(const std::string& name,
                                     const std::string& doc_string = "")
  {
    auto& param = GetParam(name);
    param = ParameterBlock(name, chi_data_types::Varying::DefaultValue<T>());
    parameter_class_tags_[name] = InputParameterTag::REQUIRED;
    if (not doc_string.empty()) parameter_doc_string_[name] = doc_string;
  }

public:
  /**\brief Assigns parameters with thorough type checks, deprecation checks,
   * unused parameter checks.*/
  void AssignParameters(const ParameterBlock& params);

  /**Returns the raw parameter block used at assignment. This can be used
   * to see if a user supplied an optional parameter or not.*/
  const ParameterBlock& ParametersAtAssignment() const
  {
    return param_block_at_assignment_;
  }

  void
  MarkParamaterDeprecatedWarning(const std::string& param_name,
                                 const std::string& deprecation_message = "");
  void
  MarkParamaterDeprecatedError(const std::string& param_name,
                               const std::string& deprecation_message = "");
  void MarkParamaterRenamed(const std::string& param_name,
                            const std::string& renaming_description);
  void ConstrainParameterRange(const std::string& param_name,
                               AllowableRangePtr allowable_range);
  /**\brief Sets a tag for the given parameter that will allow its type to be
   * mismatched upon assignment.*/
  void SetParameterTypeMismatchAllowed(const std::string& param_name);

  /**Dumps the input parameters to stdout.*/
  void DumpParameters() const;
};

} // namespace chi

#endif // CHITECH_INPUT_PARAMETERS_H
