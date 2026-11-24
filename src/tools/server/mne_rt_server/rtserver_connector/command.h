//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     command.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Typed, parameter-bearing command object that round-trips between mne-cpp clients and @c mne_rt_server.
 *
 * @ref RTSERVER::Command is the unit of conversation on the
 * @c mne_rt_server control channel after parsing has assigned types
 * (compare @ref RTSERVER::RawCommand, which carries only string tokens).
 * Each instance holds a short command keyword (e.g. @c bufsize,
 * @c selch, @c start), a human-readable description, a list of named
 * parameters with both their typed @ref RTSERVER::CommandValue values and their per-parameter
 * descriptions, and a flag selecting JSON or CLI serialisation when the
 * object goes out on the command port of mne_rt_server.
 *
 * Two construction paths matter. The data path: an incoming JSON object
 * is parsed into a fully-populated @c Command by the @c mne::JsonObject
 * constructor, so the server’s self-described command list (returned by
 * @c requestCommands()) can be replayed locally without hard-coding the
 * schema. The authoring path: client code constructs an empty
 * @c Command with name, description and parameter declarations, fills
 * @c pValues() per call site, then forwards it to
 * the command port. @c toStringReadySend() and
 * @c toJsonObject() / @c toStringList() format the same object for the
 * different transports.
 *
 * @c Command implements @ref RTSERVER::ICommand from the generic command
 * pattern so it can be dispatched through @ref RTSERVER::CommandManager without
 * the manager needing to know whether the receiver is local or remote;
 * the callbacks registered with @c connectExecuted and the manager's
 * @c connectTriggered / @c connectResponse route execution back to subscribers.
 */

#ifndef COMMAND_H
#define COMMAND_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtserver_connector_global.h"

#include "command_pattern.h"

#include <mne/base/json.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

class CommandManager;

//=============================================================================================================
/**
 * @brief Typed value of a command parameter, the standard-library counterpart of the QVariant the commands held.
 *
 * The type names are those the command port advertises in its help ("int", "uint", "float", "double", "bool" and
 * "QString" for text), so the JSON command descriptions stay the same for existing clients.
 *
 * @since 2.0.0
 */
class RTSERVER_CONNECTOR_EXPORT CommandValue
{
public:
    /** Held value; std::monostate for a value without type. */
    using Storage = std::variant<std::monostate, bool, int, unsigned int, float, double, std::string>;

    CommandValue() = default;
    CommandValue(bool value);
    CommandValue(int value);
    CommandValue(unsigned int value);
    CommandValue(float value);
    CommandValue(double value);
    CommandValue(const char* value);
    CommandValue(std::string value);

    //=========================================================================================================
    /**
     * @param[in] typeName   Type name as on the command port, e.g. "int" or "QString".
     *
     * @return The default value of that type, or a value without type if the name is unknown.
     */
    static CommandValue fromTypeName(std::string_view typeName);

    //=========================================================================================================
    /**
     * @return The type name as on the command port, or an empty string for a value without type.
     */
    std::string typeName() const;

    //=========================================================================================================
    /**
     * @return Whether the value has a type.
     */
    bool isValid() const;

    //=========================================================================================================
    /**
     * Replaces the value by @p text converted to the type of this value, as a QVariant converts a string.
     *
     * @param[in] text   Text of the new value.
     *
     * @return False, leaving the value unchanged, if the text is no value of this type or the value has no type.
     */
    bool convertFrom(std::string_view text);

    int toInt(bool* ok = nullptr) const;           /**< @return The value as int, 0 if it is none. */
    unsigned int toUInt(bool* ok = nullptr) const; /**< @return The value as unsigned int, 0 if it is none. */
    float toFloat(bool* ok = nullptr) const;       /**< @return The value as float, 0 if it is none. */
    double toDouble(bool* ok = nullptr) const;     /**< @return The value as double, 0 if it is none. */
    bool toBool() const;                           /**< @return The value as bool. */
    std::string toString() const;                  /**< @return The value as text, as QVariant::toString. */

    //=========================================================================================================
    /**
     * Replaces the value and its type.
     *
     * @param[in] value  The new value.
     */
    void setValue(const CommandValue& value);

    const Storage& storage() const; /**< @return The held value. */

    bool operator==(const CommandValue& other) const = default;

private:
    Storage m_value; /**< The held value. */
};

//=============================================================================================================
/**
 * @brief Named, parameterised command exchanged with @c mne_rt_server; supports both JSON and CLI serialisation.
 *
 * Holds the command keyword, description, parallel lists of parameter
 * names / values / descriptions, and a JSON-vs-CLI flag. Constructible
 * either from a parsed @c mne::JsonObject (when echoing the server's
 * advertised command list) or from explicit string/value lists (when
 * authoring a command locally). Execution runs the callbacks registered
 * with @c connectExecuted; @c reply and @c send are routed through the
 * owning @ref CommandManager.
 */
class RTSERVER_CONNECTOR_EXPORT Command : public ICommand
{
public:
    /** Called with the command when it is executed. */
    using ExecutedCallback = std::function<void(const Command&)>;

    //=========================================================================================================
    /**
     * Constructs an empty command.
     *
     * @param[in] p_bIsJson      If is received/should be send as JSON (optional, default true).
     */
    explicit Command(bool p_bIsJson = true);

    //=========================================================================================================
    /**
     * Constructor which parses a command stored in a json object
     *
     * @param[in] p_sCommand         Command.
     * @param[in] p_qCommandContent  Content encapsulated in a JsonObject.
     * @param[in] p_bIsJson          If is received/should be send as JSON (optional, default true).
     */
    explicit Command(const std::string& p_sCommand, const mne::JsonObject& p_qCommandContent, bool p_bIsJson = true);

    //=========================================================================================================
    /**
     * Constructs a command without parameters
     *
     * @param[in] p_sCommand         Command.
     * @param[in] p_sDescription     Command description.
     * @param[in] p_bIsJson          If is received/should be send as JSON (optional, default true).
     */
    explicit Command(const std::string& p_sCommand, const std::string& p_sDescription, bool p_bIsJson = true);

    //=========================================================================================================
    /**
     * Constructs a command with parameters; the parameter descriptions are left empty.
     *
     * @param[in] p_sCommand             Command.
     * @param[in] p_sDescription         Command description.
     * @param[in] p_qListParamNames      Parameter names.
     * @param[in] p_qListParamValues     Parameter values/types.
     * @param[in] p_bIsJson              If is received/should be send as JSON (optional, default true).
     */
    explicit Command(const std::string& p_sCommand, const std::string& p_sDescription,
                     const mne::StringList& p_qListParamNames, const std::vector<CommandValue>& p_qListParamValues, bool p_bIsJson = true);

    //=========================================================================================================
    /**
     * Constructs a command with parameters and parameter descriptions.
     *
     * @param[in] p_sCommand                 Command.
     * @param[in] p_sDescription             Command description.
     * @param[in] p_qListParamNames          Parameter names.
     * @param[in] p_qListParamValues         Parameter values/types.
     * @param[in] p_vecParameterDescriptions Parameter descriptions.
     * @param[in] p_bIsJson                  If is received/should be send as JSON (optional, default true).
     */
    explicit Command(const std::string& p_sCommand, const std::string& p_sDescription,
                     const mne::StringList& p_qListParamNames, const std::vector<CommandValue>& p_qListParamValues, const mne::StringList& p_vecParameterDescriptions, bool p_bIsJson = true);

    //=========================================================================================================
    /**
     * Copy constructor. Copies the data and the owning manager, not the registered callbacks.
     *
     * @param[in] p_Command      Command which should be copied.
     */
    Command(const Command& p_Command);

    ~Command() override;

    //=========================================================================================================
    /**
     * @return The command keyword.
     */
    inline const std::string& command() const;

    //=========================================================================================================
    /**
     * @return The number of parameters.
     */
    inline std::uint32_t count() const;

    //=========================================================================================================
    /**
     * @return The command description.
     */
    inline const std::string& description() const;

    //=========================================================================================================
    /**
     * Runs the callbacks registered with @ref connectExecuted, in the order they were registered.
     */
    void execute() override;

    //=========================================================================================================
    /**
     * Registers a callback that runs when the command is executed.
     *
     * @param[in] callback   Called with this command.
     */
    void connectExecuted(ExecutedCallback callback);

    //=========================================================================================================
    /**
     * @return Whether the command was received in (or should be sent as) JSON; assignable.
     */
    inline bool& isJson();

    //=========================================================================================================
    /**
     * @return Whether the command was received in (or should be sent as) JSON.
     */
    inline bool isJson() const;

    //=========================================================================================================
    /**
     * @return The parameter descriptions.
     */
    inline const mne::StringList& pDescriptions() const;

    //=========================================================================================================
    /**
     * @return The parameter names.
     */
    inline const mne::StringList& pNames() const;

    //=========================================================================================================
    /**
     * @return The parameter values.
     */
    inline std::vector<CommandValue>& pValues();

    //=========================================================================================================
    /**
     * @return The parameter values.
     */
    inline const std::vector<CommandValue>& pValues() const;

    //=========================================================================================================
    /**
     * Sends a reply through the response channel of the owning @ref CommandManager, if any.
     *
     * @param[in] p_sReply   Reply text.
     */
    void reply(const std::string& p_sReply);

    //=========================================================================================================
    /**
     * Announces this command through the triggered channel of the owning @ref CommandManager, if any.
     */
    void send();

    //=========================================================================================================
    /**
     * Sets the manager whose response and triggered channels @ref reply and @ref send use.
     *
     * @param[in] p_pManager     The owning manager, or nullptr.
     */
    inline void setManager(CommandManager* p_pManager);

    //=========================================================================================================
    /**
     * @return The command description and its parameters (description and type) as JSON object.
     */
    mne::JsonObject toJsonObject() const;

    //=========================================================================================================
    /**
     * @return The command, its bracketed parameter descriptions and its description.
     */
    mne::StringList toStringList() const;

    //=========================================================================================================
    /**
     * @return The command and its parameter values formatted as "command":{"name":"value",...}.
     */
    std::string toStringReadySend() const;

    //=========================================================================================================
    /**
     * Assignment operator. Copies the command, the description and the parameters; keeps the JSON flag, the owning
     * manager and the registered callbacks of this command.
     *
     * @param[in] rhs    Command which should be assigned.
     *
     * @return Reference to this object.
     */
    Command& operator=(const Command& rhs);

    //=========================================================================================================
    /**
     * @param[in] key    Parameter name.
     *
     * @return The value of the parameter, or a shared invalid value if there is none.
     */
    CommandValue& operator[](const std::string& key);

    //=========================================================================================================
    /**
     * @param[in] idx    Parameter index.
     *
     * @return The value of the parameter, or a shared invalid value if there is none.
     */
    CommandValue& operator[](std::int32_t idx);

    //=========================================================================================================
    /**
     * @param[in] key    Parameter name.
     *
     * @return The value of the parameter, or an invalid value if there is none.
     */
    CommandValue operator[](const std::string& key) const;

public:
    std::string m_sCommand;                       /**< The command keyword. */
    std::string m_sDescription;                   /**< Human-readable description of the command. */
    mne::StringList m_qListParamNames;            /**< Parameter names (positional order). */
    std::vector<CommandValue> m_qListParamValues; /**< Current parameter values (same order as names). */
    mne::StringList m_qListParamDescriptions;     /**< Per-parameter description strings. */
    bool m_bIsJson;                               /**< True when the command was parsed from JSON. */

private:
    CommandManager* m_pManager = nullptr;     /**< Owning manager, routes replies; not owned. */
    std::vector<ExecutedCallback> m_executed; /**< Callbacks of @ref execute. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const std::string& Command::command() const
{
    return m_sCommand;
}

//=============================================================================================================

inline std::uint32_t Command::count() const
{
    return static_cast<std::uint32_t>(m_qListParamValues.size());
}

//=============================================================================================================

inline const std::string& Command::description() const
{
    return m_sDescription;
}

//=============================================================================================================

inline bool& Command::isJson()
{
    return m_bIsJson;
}

//=============================================================================================================

inline bool Command::isJson() const
{
    return m_bIsJson;
}

//=============================================================================================================

inline const mne::StringList& Command::pDescriptions() const
{
    return m_qListParamDescriptions;
}

//=============================================================================================================

inline const mne::StringList& Command::pNames() const
{
    return m_qListParamNames;
}

//=============================================================================================================

inline std::vector<CommandValue>& Command::pValues()
{
    return m_qListParamValues;
}

//=============================================================================================================

inline const std::vector<CommandValue>& Command::pValues() const
{
    return m_qListParamValues;
}

//=============================================================================================================

inline void Command::setManager(CommandManager* p_pManager)
{
    m_pManager = p_pManager;
}

} // NAMESPACE

#endif // COMMAND_H
