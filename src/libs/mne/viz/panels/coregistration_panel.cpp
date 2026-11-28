//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     coregistration_panel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    CoregistrationPanel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "coregistration_panel.h"
#include <mne/base/string_utils.h>
#include <mne/fiff/fiff_constants.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdio>
#include <numbers>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

namespace
{

constexpr const char* kBem = "bem";
constexpr const char* kPicking = "pickFiducials";
constexpr const char* kFiducial = "fiducial";
constexpr const char* kFidX = "fiducialX";
constexpr const char* kFidY = "fiducialY";
constexpr const char* kFidZ = "fiducialZ";
constexpr const char* kFidFile = "fiducialFile";
constexpr const char* kFidStoreFile = "fiducialStoreFile";
constexpr const char* kDigFile = "digitizerFile";
constexpr const char* kTransFile = "transFile";
constexpr const char* kTransStoreFile = "transStoreFile";
constexpr const char* kWeightLpa = "weightLpa";
constexpr const char* kWeightNas = "weightNasion";
constexpr const char* kWeightRpa = "weightRpa";
constexpr const char* kUseEeg = "useEeg";
constexpr const char* kWeightEeg = "weightEeg";
constexpr const char* kUseHpi = "useHpi";
constexpr const char* kWeightHpi = "weightHpi";
constexpr const char* kUseHsp = "useHsp";
constexpr const char* kWeightHsp = "weightHsp";
constexpr const char* kMaxDist = "maxDistance";
constexpr const char* kOmitted = "omittedPoints";
constexpr const char* kAutoScale = "autoScale";
constexpr const char* kMaxIter = "maxIterations";
constexpr const char* kConvergence = "convergence";
constexpr const char* kRmse = "rmse";
constexpr const char* kFitFiducials = "fitFiducials";
constexpr const char* kFitIcp = "fitIcp";
constexpr const char* kScalingMode = "scalingMode";
constexpr const char* kScaleAny = "scaleAny";
constexpr const char* kScaleAxes = "scaleAxes";
constexpr std::array<const char*, 3> kScale = {"scaleX", "scaleY", "scaleZ"};
constexpr std::array<const char*, 3> kRotation = {"rotationX", "rotationY", "rotationZ"};
constexpr std::array<const char*, 3> kTranslation = {"translationX", "translationY", "translationZ"};

constexpr const char* kUniform = "Uniform";
constexpr const char* kThreeAxis = "3-Axis";

constexpr const char* kSubject = "MRI Subject";
constexpr const char* kFiducials = "MRI Fiducials";
constexpr const char* kFiles = "Files";
constexpr const char* kDigitizerOptions = "Digitizer Options";
constexpr const char* kFitting = "Fitting Options";
constexpr const char* kScaling = "Scaling";
constexpr const char* kAdjustment = "Adjustment";

SettingField make(const char* key, SettingField::Kind kind, const std::string& label, SettingValue value, const char* group,
                  const std::string& tooltip = {})
{
    SettingField field;
    field.key = key;
    field.kind = kind;
    field.label = label;
    field.default_value = std::move(value);
    field.group = group;
    field.tooltip = tooltip;
    // The co-registration starts afresh each session
    field.persistent = false;
    return field;
}

SettingField number(const char* key, const std::string& label, double value, double minimum, double maximum, double step, int decimals,
                    const std::string& unit, const char* group, const std::string& tooltip)
{
    SettingField field = make(key, SettingField::Kind::Double, label, value, group, tooltip);
    field.minimum = minimum;
    field.maximum = maximum;
    field.step = step;
    field.decimals = decimals;
    field.unit = unit;
    return field;
}

SettingField path(const char* key, const std::string& label, const std::string& filter, bool save, const std::string& tooltip)
{
    SettingField field = make(key, SettingField::Kind::Path, label, std::string(), kFiles, tooltip);
    field.file_filter = filter;
    field.save_path = save;
    // Typed paths are used once they are complete
    field.commit = SettingField::Commit::WhenFinished;
    return field;
}

std::string millimeters(double meters)
{
    std::array<char, 64> text{};
    std::snprintf(text.data(), text.size(), "%.2f mm", meters * 1000.0);
    return text.data();
}

std::size_t fiducial_index(int fiducial)
{
    if (fiducial == FIFFV_POINT_NASION) {
        return 1;
    }
    return fiducial == FIFFV_POINT_RPA ? 2 : 0;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CoregistrationPanel::CoregistrationPanel()
: Panel("CoregSettingsView", "Co-Registration")
, m_fiducials{Eigen::Vector3f::Zero(), Eigen::Vector3f::Zero(), Eigen::Vector3f::Zero()}
{
    set_width(330);
    SettingsForm& f = form();
    f.add_flag(kScaleAny, false);
    f.add_flag(kScaleAxes, false);

    f.add(make(kBem, SettingField::Kind::Choice, "BEM:", std::string(), kSubject,
               "Select the Bem to use for coregistration. Load the Bem via: File->Open"));

    f.add(make(kPicking, SettingField::Kind::Bool, "Pick Fiducials from View", false, kFiducials));
    SettingField fiducial = make(kFiducial, SettingField::Kind::Choice, "Fiducial:", std::to_string(FIFFV_POINT_LPA), kFiducials);
    fiducial.choices = {{std::to_string(FIFFV_POINT_LPA), "LPA"}, {std::to_string(FIFFV_POINT_NASION), "Nasion"}, {std::to_string(FIFFV_POINT_RPA), "RPA"}};
    fiducial.enabled_if = kPicking;
    f.add(std::move(fiducial));
    for (const auto& [key, label] : {std::pair{kFidX, "x:"}, std::pair{kFidY, "y:"}, std::pair{kFidZ, "z:"}}) {
        SettingField coordinate = make(key, SettingField::Kind::Info, label, millimeters(0.0), kFiducials);
        coordinate.enabled_if = kPicking;
        f.add(std::move(coordinate));
    }

    f.add(path(kFidFile, "Load fiducials:", "Fif Files (*.fif)", false, "Optional. Load the fiducials from file."));
    f.add(path(kFidStoreFile, "Store fiducials:", "Fif file (*fiducials.fif)", true, "Store the fiducials to file."));
    f.add(path(kDigFile, "Load digitizers:", "Fif Files (*.fif)", false, "Load the digitizers from file."));
    f.add(path(kTransFile, "Load transformation:", "Fif Files (*-trans.fif)", false, "Optional. Load the coordinate transformation from file."));
    f.add(path(kTransStoreFile, "Store transformation:", "Fif file (*-trans.fif)", true, "Store the coordinate transformation to file."));

    f.add(number(kWeightLpa, "LPA weight:", 1.0, 0.0, 99.99, 1.0, 2, {}, kDigitizerOptions, "The weight for the LPA."));
    f.add(number(kWeightNas, "Nasion weight:", 10.0, 0.0, 99.99, 1.0, 2, {}, kDigitizerOptions, "The weight for the Nasion."));
    f.add(number(kWeightRpa, "RPA weight:", 1.0, 0.0, 99.99, 1.0, 2, {}, kDigitizerOptions, "The weight for the RPA."));
    f.add(make(kUseEeg, SettingField::Kind::Bool, "EEG", false, kDigitizerOptions, "Whether to use the EEG points for the Coregistration."));
    f.add(number(kWeightEeg, "EEG weight:", 1.0, 0.0, 99.99, 1.0, 2, {}, kDigitizerOptions, "The weight for the EEG points."));
    f.add(make(kUseHpi, SettingField::Kind::Bool, "HPI", true, kDigitizerOptions, "Whether to use the HPI points for the Coregistration."));
    f.add(number(kWeightHpi, "HPI weight:", 1.0, 0.0, 99.99, 1.0, 2, {}, kDigitizerOptions, "The weight for the HPI points."));
    f.add(make(kUseHsp, SettingField::Kind::Bool, "HSP", true, kDigitizerOptions,
               "Whether to use the HSP points for the Coregistration. HSP = Head Shape Points"));
    f.add(number(kWeightHsp, "HSP weight:", 1.0, 0.0, 99.99, 1.0, 2, {}, kDigitizerOptions, "The weight for the HSP points. HSP = Head Shape Points"));
    SettingField distance = make(kMaxDist, SettingField::Kind::Int, "Max. distance to surface:", 10, kDigitizerOptions,
                                 "The maximum allowed distace between head surface and digitizer cloud. This is used to discard outliers.");
    distance.maximum = 99;
    distance.unit = " mm";
    f.add(std::move(distance));
    f.add(make(kOmitted, SettingField::Kind::Info, "Points Omitted:", std::string("0"), kDigitizerOptions));

    f.add(make(kAutoScale, SettingField::Kind::Bool, "Apply automatic scaling", false, kFitting,
               "Whether to use automatic scaling for the fiducial alignment."));
    SettingField iterations = make(kMaxIter, SettingField::Kind::Int, "ICP iterations (max):", 20, kFitting,
                                   "The maximum number of iterations for the ICP algorithm.");
    iterations.maximum = 99;
    f.add(std::move(iterations));
    f.add(number(kConvergence, "Convergence Limit:", 0.01, 0.0, 99.99, 0.1, 2, " mm", kFitting, "The convergence limit for the ICP algorithm."));
    f.add(make(kRmse, SettingField::Kind::Info, "RMSE:", std::string("0 mm"), kFitting,
               "The Root-Mean-Square-Error of the distance between closest point and digigizer in mm"));
    f.add(make(kFitFiducials, SettingField::Kind::Action, "Fit Fiducials", false, kFitting,
               "Fiducial alignment. Apply this step before using the ICP algorithm to get a better first guess."));
    f.add(make(kFitIcp, SettingField::Kind::Action, "Fit ICP", false, kFitting, "Co-Registration with the ICP algorithm."));

    SettingField mode = make(kScalingMode, SettingField::Kind::Choice, "Scaling Mode:", std::string("None"), kScaling,
                             "The scaling Mode. None - No scaling is applied; Uniform - same scaling for x,y,z-axis; 3-Axis - scaling on each axis.");
    mode.choices = {{"None", "None"}, {kUniform, kUniform}, {kThreeAxis, kThreeAxis}};
    f.add(std::move(mode));
    constexpr std::array<const char*, 3> kAxes = {"X", "Y", "Z"};
    for (std::size_t i = 0; i < 3; ++i) {
        SettingField scale = number(kScale[i], std::string("Scaling ") + kAxes[i] + ":", 1.0, 0.0, 99.99, 0.01, 3, {}, kScaling,
                                    std::string("Scaling to apply in ") + kAxes[i] + "-direction.");
        scale.enabled_if = i == 0 ? kScaleAny : kScaleAxes;
        f.add(std::move(scale));
    }
    for (std::size_t i = 0; i < 3; ++i) {
        f.add(number(kRotation[i], std::string("Rotation ") + kAxes[i] + ":", 0.0, -180.0, 180.0, 0.1, 2, " °", kAdjustment,
                     std::string("Rotation arround ") + kAxes[i] + "-axis."));
    }
    for (std::size_t i = 0; i < 3; ++i) {
        f.add(number(kTranslation[i], std::string("Translation ") + kAxes[i] + ":", 0.0, -99.99, 99.99, 0.1, 2, " mm", kAdjustment,
                     std::string("Translation to apply in ") + kAxes[i] + "-direction."));
    }

    set_bems({});

    m_listener = f.add_listener([this](const std::string& key, const std::optional<SettingValue>& value) {
        if (!m_updating && (value || key == kFitFiducials || key == kFitIcp)) {
            changed(key);
        }
    });
}

//=============================================================================================================

CoregistrationPanel::~CoregistrationPanel()
{
    form().remove_listener(m_listener);
}

//=============================================================================================================

void CoregistrationPanel::set_silently(const std::string& key, const SettingValue& value)
{
    const bool updating = std::exchange(m_updating, true);
    form().set_value(key, value);
    m_updating = updating;
}

//=============================================================================================================

void CoregistrationPanel::changed(const std::string& key)
{
    if (key == kBem) {
        if (m_callbacks.bem_changed) {
            m_callbacks.bem_changed(bem());
        }
    } else if (key == kPicking) {
        if (m_callbacks.picking_changed) {
            m_callbacks.picking_changed(picking());
        }
    } else if (key == kFiducial) {
        show_fiducial();
        if (m_callbacks.fiducial_changed) {
            m_callbacks.fiducial_changed(fiducial());
        }
    } else if (key == kFidFile) {
        report_path(key, m_callbacks.fiducials_loaded, false);
    } else if (key == kFidStoreFile) {
        report_path(key, m_callbacks.fiducials_stored, true);
    } else if (key == kDigFile) {
        report_path(key, m_callbacks.digitizers_loaded, false);
    } else if (key == kTransFile) {
        report_path(key, m_callbacks.transformation_loaded, false);
    } else if (key == kTransStoreFile) {
        report_path(key, m_callbacks.transformation_stored, true);
    } else if (key == kFitFiducials || key == kFitIcp) {
        stop_picking();
        const std::function<void()>& fit = key == kFitFiducials ? m_callbacks.fit_fiducials : m_callbacks.fit_icp;
        if (fit) {
            fit();
        }
    } else if (key == kScalingMode) {
        update_scaling_mode();
    } else if (key.starts_with("scale") || key.starts_with("rotation") || key.starts_with("translation")) {
        if (m_callbacks.transformation_changed) {
            m_callbacks.transformation_changed();
        }
    }
}

//=============================================================================================================

void CoregistrationPanel::report_path(const std::string& key, const std::function<void(const std::filesystem::path&)>& callback, bool clear)
{
    const std::string file = form().get<std::string>(key);
    if (file.empty()) {
        return;
    }
    // A cleared store path lets the user store to the same file again
    if (clear) {
        set_silently(key, std::string());
    }
    if (callback) {
        callback(path_from_utf8(file));
    }
}

//=============================================================================================================

void CoregistrationPanel::stop_picking()
{
    if (picking()) {
        set_silently(kPicking, false);
        if (m_callbacks.picking_changed) {
            m_callbacks.picking_changed(false);
        }
    }
}

//=============================================================================================================

void CoregistrationPanel::show_fiducial()
{
    const Eigen::Vector3f& position = m_fiducials[fiducial_index(fiducial())];
    set_silently(kFidX, millimeters(position.x()));
    set_silently(kFidY, millimeters(position.y()));
    set_silently(kFidZ, millimeters(position.z()));
}

//=============================================================================================================

void CoregistrationPanel::update_scaling_mode()
{
    const std::string mode = form().get<std::string>(kScalingMode);
    set_silently(kScaleAny, mode == kUniform || mode == kThreeAxis);
    set_silently(kScaleAxes, mode == kThreeAxis);
}

//=============================================================================================================

void CoregistrationPanel::set_bems(const std::vector<std::string>& names)
{
    std::vector<SettingChoice> choices;
    choices.reserve(names.size() + 1);
    for (const std::string& name : names) {
        choices.push_back({name, name});
    }
    if (choices.empty()) {
        choices.push_back({"", "Select BEM"});
    }
    const bool updating = std::exchange(m_updating, true);
    form().set_choices(kBem, std::move(choices));
    m_updating = updating;
}

//=============================================================================================================

std::string CoregistrationPanel::bem() const
{
    return form().get<std::string>(kBem);
}

//=============================================================================================================

void CoregistrationPanel::set_fiducial(const Eigen::Vector3f& position)
{
    const int current = fiducial();
    m_fiducials[fiducial_index(current)] = position;
    int next = FIFFV_POINT_LPA;
    if (current == FIFFV_POINT_LPA) {
        next = FIFFV_POINT_NASION;
    } else if (current == FIFFV_POINT_NASION) {
        next = FIFFV_POINT_RPA;
    }
    set_silently(kFiducial, std::to_string(next));
    show_fiducial();
}

//=============================================================================================================

int CoregistrationPanel::fiducial() const
{
    return std::stoi(form().get<std::string>(kFiducial, std::to_string(FIFFV_POINT_LPA)));
}

//=============================================================================================================

bool CoregistrationPanel::picking() const
{
    return form().get<bool>(kPicking);
}

//=============================================================================================================

void CoregistrationPanel::set_omitted_points(int points)
{
    set_silently(kOmitted, std::to_string(points));
}

//=============================================================================================================

void CoregistrationPanel::set_rmse(float rmse)
{
    set_silently(kRmse, millimeters(rmse));
}

//=============================================================================================================

int CoregistrationPanel::max_iterations() const
{
    return form().get<int>(kMaxIter);
}

//=============================================================================================================

float CoregistrationPanel::convergence() const
{
    return static_cast<float>(form().get<double>(kConvergence) / 1000.0);
}

//=============================================================================================================

bool CoregistrationPanel::auto_scale() const
{
    return form().get<bool>(kAutoScale);
}

//=============================================================================================================

float CoregistrationPanel::omit_distance() const
{
    return static_cast<float>(form().get<int>(kMaxDist)) / 1000.0f;
}

//=============================================================================================================

float CoregistrationPanel::weight_lpa() const
{
    return static_cast<float>(form().get<double>(kWeightLpa));
}

//=============================================================================================================

float CoregistrationPanel::weight_rpa() const
{
    return static_cast<float>(form().get<double>(kWeightRpa));
}

//=============================================================================================================

float CoregistrationPanel::weight_nasion() const
{
    return static_cast<float>(form().get<double>(kWeightNas));
}

//=============================================================================================================

float CoregistrationPanel::weight_eeg() const
{
    return static_cast<float>(form().get<double>(kWeightEeg));
}

//=============================================================================================================

float CoregistrationPanel::weight_hsp() const
{
    return static_cast<float>(form().get<double>(kWeightHsp));
}

//=============================================================================================================

float CoregistrationPanel::weight_hpi() const
{
    return static_cast<float>(form().get<double>(kWeightHpi));
}

//=============================================================================================================

std::vector<int> CoregistrationPanel::digitizer_kinds() const
{
    // The fiducials are always used
    std::vector<int> kinds{FIFFV_POINT_CARDINAL};
    kinds.reserve(4);
    if (form().get<bool>(kUseEeg)) {
        kinds.push_back(FIFFV_POINT_EEG);
    }
    if (form().get<bool>(kUseHpi)) {
        kinds.push_back(FIFFV_POINT_HPI);
    }
    if (form().get<bool>(kUseHsp)) {
        kinds.push_back(FIFFV_POINT_EXTRA);
    }
    return kinds;
}

//=============================================================================================================

void CoregistrationPanel::transformation(Eigen::Vector3f& rotation, Eigen::Vector3f& translation, Eigen::Vector3f& scaling) const
{
    constexpr double kRadians = std::numbers::pi / 180.0;
    for (int i = 0; i < 3; ++i) {
        const auto axis = static_cast<std::size_t>(i);
        translation(i) = static_cast<float>(form().get<double>(kTranslation[axis]) / 1000.0);
        // Euler order: z, y, x
        rotation(2 - i) = static_cast<float>(form().get<double>(kRotation[axis]) * kRadians);
    }
    const std::string mode = form().get<std::string>(kScalingMode);
    if (mode == kUniform) {
        scaling.fill(static_cast<float>(form().get<double>(kScale[0])));
    } else if (mode == kThreeAxis) {
        for (int i = 0; i < 3; ++i) {
            scaling(i) = static_cast<float>(form().get<double>(kScale[static_cast<std::size_t>(i)]));
        }
    } else {
        scaling.fill(1.0f);
    }
}

//=============================================================================================================

void CoregistrationPanel::set_transformation(const Eigen::Vector3f& translation, const Eigen::Vector3f& rotation, const Eigen::Vector3f& scaling)
{
    constexpr double kDegrees = 180.0 / std::numbers::pi;
    for (int i = 0; i < 3; ++i) {
        const auto axis = static_cast<std::size_t>(i);
        set_silently(kTranslation[axis], static_cast<double>(translation(i)) * 1000.0);
        set_silently(kRotation[axis], static_cast<double>(rotation(2 - i)) * kDegrees);
        set_silently(kScale[axis], static_cast<double>(scaling(i)));
    }
}
