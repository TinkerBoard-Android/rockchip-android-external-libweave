// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "buffet/device_registration_info.h"

#include <base/json/json_reader.h>
#include <base/json/json_writer.h>
#include <base/values.h>
#include <base/file_util.h>
#include <memory>

#include "buffet/http_utils.h"
#include "buffet/mime_utils.h"
#include "buffet/string_utils.h"
#include "buffet/data_encoding.h"

using namespace chromeos::http;
using namespace chromeos::data_encoding;

namespace {
// Persistent keys
const char kClientId[]      = "client_id";
const char kClientSecret[]  = "client_secret";
const char kApiKey[]        = "api_key";
const char kRefreshToken[]  = "refresh_token";
const char kDeviceId[]      = "device_id";
const char kOAuthURL[]      = "oauth_url";
const char kServiceURL[]    = "service_url";
const char kRobotAccount[]  = "robot_account";
// Transient keys
const char kDeviceKind[]    = "device_kind";
const char kSystemName[]    = "system_name";
const char kDisplayName[]   = "display_name";

const base::FilePath::CharType kDeviceInfoFilePath[] =
    FILE_PATH_LITERAL("/var/lib/buffet/device_reg_info");

bool GetParamValue(
  const std::map<std::string, std::shared_ptr<base::Value>>& params,
  const std::string& param_name,
  std::string* param_value) {
  auto p = params.find(param_name);
  if (p == params.end())
    return false;

  return p->second->GetAsString(param_value);
}

std::pair<std::string, std::string> BuildAuthHeader(
    const std::string& access_token_type,
    const std::string& access_token) {
  std::string authorization = chromeos::string_utils::Join(' ',
                                                           access_token_type,
                                                           access_token);
  return {request_header::kAuthorization, authorization};
}

std::unique_ptr<base::DictionaryValue> ParseOAuthResponse(
  const Response* response, std::string* error_message) {
  int code = 0;
  auto resp = ParseJsonResponse(response, &code, error_message);
  if (resp && code >= status_code::BadRequest) {
    if (error_message) {
      error_message->clear();
      std::string error_code, error;
      if (resp->GetString("error", &error_code) &&
          resp->GetString("error_description", &error)) {
        *error_message = error_code + " (" + error + ")";
      } else {
        *error_message = "Unexpected OAuth error";
      }
    }
    return std::unique_ptr<base::DictionaryValue>();
  }
  return resp;
}

std::string BuildURL(std::string url,
                     const std::string& subpath,
                     const WebParamList& params) {
  if (!subpath.empty()) {
    if (!url.empty() && url.back() != '/')
      url += '/';
    url += subpath;
  }

  if (!params.empty()) {
    url += '?';
    url += WebParamsEncode(params);
  }
  return url;
}


} // anonymous namespace

namespace buffet {

std::pair<std::string, std::string>
    DeviceRegistrationInfo::GetAuthorizationHeader() const {
  return BuildAuthHeader(/*"Bearer"*/"OAuth", access_token_);
}

std::string DeviceRegistrationInfo::GetServiceURL(
    const std::string& subpath, const WebParamList& params) const {
  return BuildURL(service_url_, subpath, params);
}

std::string DeviceRegistrationInfo::GetDeviceURL(
    const std::string& subpath, const WebParamList& params) const {
  CHECK(!device_id_.empty()) << "Must have a valid device ID";
  std::string path = "devices/" + device_id_;
  if (!subpath.empty()) {
    path += '/' + subpath;
  }
  return GetServiceURL(path, params);
}

std::string DeviceRegistrationInfo::GetOAuthURL(const std::string& subpath,
                                    const WebParamList& params) const {
  return BuildURL(oauth_url_, subpath, params);
}

std::string DeviceRegistrationInfo::GetDeviceId() {
  return CheckRegistration() ? device_id_ : std::string();
}

bool DeviceRegistrationInfo::Load() {
  // TODO(avakulenko): Figure out security implications of storing
  // this data unencrypted.
  std::string json;
  if (!base::ReadFileToString(base::FilePath(kDeviceInfoFilePath), &json))
    return false;

  auto value = std::unique_ptr<base::Value>(base::JSONReader::Read(json));
  const base::DictionaryValue* dict = nullptr;
  if (!value || !value->GetAsDictionary(&dict))
    return false;

  // Get the values into temp variables first to make sure we can get
  // all the data correctly before changing the state of this object.
  std::string client_id;
  if (!dict->GetString(kClientId, &client_id))
    return false;
  std::string client_secret;
  if (!dict->GetString(kClientSecret, &client_secret))
    return false;
  std::string api_key;
  if (!dict->GetString(kApiKey, &api_key))
    return false;
  std::string refresh_token;
  if (!dict->GetString(kRefreshToken, &refresh_token))
    return false;
  std::string device_id;
  if (!dict->GetString(kDeviceId, &device_id))
    return false;
  std::string oauth_url;
  if (!dict->GetString(kOAuthURL, &oauth_url))
    return false;
  std::string service_url;
  if (!dict->GetString(kServiceURL, &service_url))
    return false;
  std::string device_robot_account;
  if (!dict->GetString(kRobotAccount, &device_robot_account))
    return false;

  client_id_            = client_id;
  client_secret_        = client_secret;
  api_key_              = api_key;
  refresh_token_        = refresh_token;
  device_id_            = device_id;
  oauth_url_            = oauth_url;
  service_url_          = service_url;
  device_robot_account_ = device_robot_account;
  return true;
}

bool DeviceRegistrationInfo::Save() const {
  // TODO(avakulenko): Figure out security implications of storing
  // this data unencrypted.
  base::DictionaryValue dict;
  dict.SetString(kClientId,     client_id_);
  dict.SetString(kClientSecret, client_secret_);
  dict.SetString(kApiKey,       api_key_);
  dict.SetString(kRefreshToken, refresh_token_);
  dict.SetString(kDeviceId,     device_id_);
  dict.SetString(kOAuthURL,     oauth_url_);
  dict.SetString(kServiceURL,   service_url_);
  dict.SetString(kRobotAccount, device_robot_account_);

  std::string json;
  base::JSONWriter::WriteWithOptions(&dict,
                                     base::JSONWriter::OPTIONS_PRETTY_PRINT,
                                     &json);
  int count = file_util::WriteFile(base::FilePath(kDeviceInfoFilePath),
                                   json.data(), static_cast<int>(json.size()));

  return (count > 0);
}

bool DeviceRegistrationInfo::CheckRegistration() {
  LOG(INFO) << "Checking device registration record.";
  if (refresh_token_.empty() ||
      device_id_.empty() ||
      device_robot_account_.empty()) {
    LOG(INFO) << "No valid device registration record found.";
    return false;
  }

  LOG(INFO) << "Device registration record found.";
  return ValidateAndRefreshAccessToken();
}

bool DeviceRegistrationInfo::ValidateAndRefreshAccessToken() {
  LOG(INFO) << " Checking access token expiration.";
  if (!access_token_.empty() &&
      !access_token_expiration_.is_null() &&
      access_token_expiration_ > base::Time::Now()) {
    LOG(INFO) << "Access token is still valid.";
    return true;
  }

  auto response = PostFormData(GetOAuthURL("token"), {
    {"refresh_token", refresh_token_},
    {"client_id", client_id_},
    {"client_secret", client_secret_},
    {"grant_type", "refresh_token"},
  });
  if (!response)
    return false;

  std::string error;
  auto json = ParseOAuthResponse(response.get(), &error);
  if (!json) {
    LOG(ERROR) << "Unable to refresh access token: " << error;
    return false;
  }

  int expires_in = 0;
  if (!json->GetString("access_token", &access_token_) ||
      !json->GetInteger("expires_in", &expires_in) ||
      access_token_.empty() ||
      expires_in <= 0) {
    LOG(ERROR) << "Access token unavailable.";
    return false;
  }

  access_token_expiration_ = base::Time::Now() +
                             base::TimeDelta::FromSeconds(expires_in);

  LOG(INFO) << "Access token is refreshed for additional " << expires_in
            << " seconds.";
  return true;
}

std::unique_ptr<base::Value> DeviceRegistrationInfo::GetDeviceInfo() {
  if (!CheckRegistration())
    return std::unique_ptr<base::Value>();

  auto response = Get(GetDeviceURL(), {GetAuthorizationHeader()});
  int status_code = 0;
  std::unique_ptr<base::Value> device_info =
      ParseJsonResponse(response.get(), &status_code, nullptr);

  if (device_info) {
    if (status_code >= status_code::BadRequest) {
      LOG(WARNING) << "Failed to retrieve the device info. Response code = "
                   << status_code;
      return std::unique_ptr<base::Value>();
    }
  }
  return device_info;
}

bool CheckParam(const std::string& param_name,
                const std::string& param_value,
                std::string* error_msg) {
  if (!param_value.empty())
    return true;

  if (error_msg)
    *error_msg = "Parameter " + param_name + " not specified";
  return false;
}

std::string DeviceRegistrationInfo::StartRegistration(
    const std::map<std::string, std::shared_ptr<base::Value>>& params,
  std::string* error_msg) {
  GetParamValue(params, kClientId, &client_id_);
  GetParamValue(params, kClientSecret, &client_secret_);
  GetParamValue(params, kApiKey, &api_key_);
  GetParamValue(params, kDeviceId, &device_id_);
  GetParamValue(params, kDeviceKind, &device_kind_);
  GetParamValue(params, kSystemName, &system_name_);
  GetParamValue(params, kDisplayName, &display_name_);
  GetParamValue(params, kOAuthURL, &oauth_url_);
  GetParamValue(params, kServiceURL, &service_url_);

  if (!CheckParam(kClientId, client_id_, error_msg))
    return std::string();
  if (!CheckParam(kClientSecret, client_secret_, error_msg))
    return std::string();
  if (!CheckParam(kApiKey, api_key_, error_msg))
    return std::string();
  if (!CheckParam(kDeviceKind, device_kind_, error_msg))
    return std::string();
  if (!CheckParam(kSystemName, system_name_, error_msg))
    return std::string();
  if (!CheckParam(kOAuthURL, oauth_url_, error_msg))
    return std::string();
  if (!CheckParam(kServiceURL, service_url_, error_msg))
    return std::string();

  std::vector<std::pair<std::string, std::vector<std::string>>> commands = {
    {"SetDeviceConfiguration", {"data"}}
  };

  base::DictionaryValue req_json;
  base::ListValue* set_device_configuration_params = new base::ListValue;
  base::DictionaryValue* param1 = new base::DictionaryValue;
  param1->SetString("name", "data");
  set_device_configuration_params->Append(param1);

  base::ListValue* vendor_commands = new base::ListValue;
  for (auto&& pair : commands) {
    base::ListValue* params = new base::ListValue;
    for (auto&& param_name : pair.second) {
      base::DictionaryValue* param = new base::DictionaryValue;
      param->SetString("name", param_name);
      params->Append(param);
    }
    base::DictionaryValue* command = new base::DictionaryValue;
    command->SetString("name", pair.first);
    command->Set("parameter", params);
    vendor_commands->Append(command);
  }

  req_json.SetString("oauthClientId", client_id_);
  req_json.SetString("deviceDraft.deviceKind", device_kind_);
  req_json.SetString("deviceDraft.systemName", system_name_);
  req_json.SetString("deviceDraft.displayName", display_name_);
  req_json.SetString("deviceDraft.channel.supportedType", "xmpp");
  req_json.Set("deviceDraft.commands.base.vendorCommands", vendor_commands);

  std::string url = GetServiceURL("registrationTickets", {{"key", api_key_}});
  auto resp_json = ParseJsonResponse(PostJson(url, &req_json).get(),
                                     nullptr, error_msg);
  if (!resp_json)
    return std::string();

  const base::DictionaryValue* resp_dict = nullptr;
  if (!resp_json->GetAsDictionary(&resp_dict)) {
    if (error_msg)
      *error_msg = "Invalid response received";
    return std::string();
  }

  if (!resp_dict->GetString("id", &ticket_id_))
    return std::string();

  std::string auth_url = GetOAuthURL("auth", {
    {"scope", "https://www.googleapis.com/auth/clouddevices"},
    {"redirect_uri", "urn:ietf:wg:oauth:2.0:oob"},
    {"response_type", "code"},
    {"client_id", client_id_}
  });

  base::DictionaryValue json;
  json.SetString("ticket_id", ticket_id_);
  json.SetString("auth_url", auth_url);

  std::string ret;
  base::JSONWriter::Write(&json, &ret);
  return ret;
}

bool DeviceRegistrationInfo::FinishRegistration(
    const std::string& user_auth_code) {
  if (ticket_id_.empty()) {
    LOG(ERROR) << "Finish registration without ticket ID";
    return false;
  }

  std::string url = GetServiceURL("registrationTickets/" + ticket_id_);
  std::unique_ptr<Response> response;
  if (!user_auth_code.empty()) {
    std::string user_access_token;
    response = PostFormData(GetOAuthURL("token"), {
      {"code", user_auth_code},
      {"client_id", client_id_},
      {"client_secret", client_secret_},
      {"redirect_uri", "urn:ietf:wg:oauth:2.0:oob"},
      {"grant_type", "authorization_code"}
    });
    if (!response)
      return false;

    auto json_resp = ParseOAuthResponse(response.get(), nullptr);
    if (!json_resp ||
        !json_resp->GetString("access_token", &user_access_token)) {
      return false;
    }

    base::DictionaryValue user_info;
    user_info.SetString("userEmail", "me");
    response = PatchJson(url, &user_info,
                         {BuildAuthHeader("Bearer", user_access_token)});

    std::string error;
    auto json = ParseJsonResponse(response.get(), nullptr, &error);
    if (!json) {
      LOG(ERROR) << "Error populating user info: " << error;
      return false;
    }
  }

  std::string auth_code;
  url += "/finalize?key=" + api_key_;
  do {
    LOG(INFO) << "Sending request to: " << url;
    response = PostBinary(url, nullptr, 0);
    if (response) {
      if (response->GetStatusCode() == status_code::BadRequest)
        sleep(1);
    }
  }
  while (response &&
         response->GetStatusCode() == status_code::BadRequest);
  if (response &&
      response->GetStatusCode() == status_code::Ok) {
    auto json_resp = ParseJsonResponse(response.get(), nullptr, nullptr);
    if (json_resp &&
        json_resp->GetString("robotAccountEmail", &device_robot_account_) &&
        json_resp->GetString("robotAccountAuthorizationCode", &auth_code) &&
        json_resp->GetString("deviceDraft.id", &device_id_)) {
      // Now get access_token and refresh_token
      response = PostFormData(GetOAuthURL("token"), {
        {"code", auth_code},
        {"client_id", client_id_},
        {"client_secret", client_secret_},
        {"redirect_uri", "oob"},
        {"scope", "https://www.googleapis.com/auth/clouddevices"},
        {"grant_type", "authorization_code"}
      });
      if (!response)
        return false;

      json_resp = ParseOAuthResponse(response.get(), nullptr);
      int expires_in = 0;
      if (!json_resp ||
          !json_resp->GetString("access_token", &access_token_) ||
          !json_resp->GetString("refresh_token", &refresh_token_) ||
          !json_resp->GetInteger("expires_in", &expires_in) ||
          access_token_.empty() ||
          refresh_token_.empty() ||
          expires_in <= 0) {
        LOG(ERROR) << "Access token unavailable";
        return false;
      }

      access_token_expiration_ = base::Time::Now() +
                                 base::TimeDelta::FromSeconds(expires_in);

      Save();
    }
  }
  return true;
}

}  // namespace buffet