/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2024 Z-Wave Alliance
 */
#include <cstdint>
#include <vector>
#include <iostream>
#include <iomanip>
#include <set>
extern "C" {
    #include "ZW_classcmd.h"
}

// Experimenting with frame generation using C++ (and parsing at some point).
// I would like something similar to what is available in the pytests.
// Would be nice if we could generate this from the XML file.
class CCUserCredential {
public:
  enum class operation_type_t {
    ADD    = 0x00,
    MODIFY = 0x01,
    DELETE = 0x02
  };
  enum class credential_type_t {
    PIN_CODE = 0x01,
    PASSWORD = 0x02,
    RFID_CODE = 0x03,
    BLE = 0x04,
    NFC = 0x05,
    UWB = 0x06,
    EYE_BIOMETRIC = 0x07,
    FACE_BIOMETRIC = 0x08,
    FINGER_BIOMETRIC = 0x09,
    HAND_BIOMETRIC = 0x0A,
    UNSPECIFIED_BIOMETRIC = 0x0B
  };
  class CredentialSet {
public:
    CredentialSet(void)
    {
      frame_.clear();
      uuid_ = 1;
      credential_type_ = credential_type_t::PIN_CODE;
      credential_slot_ = 1;
      operation_type_ = operation_type_t::ADD;
      credential_length_ = 0;
      credential_data_.clear();
    }
    CredentialSet & uuid(uint16_t new_uuid)
    {
      uuid_ = new_uuid;
      return *this;
    }
    CredentialSet & credential_type(credential_type_t new_credential_type)
    {
      credential_type_ = new_credential_type;
      return *this;
    }
    CredentialSet & credential_slot(uint16_t new_credential_slot)
    {
      credential_slot_ = new_credential_slot;
      return *this;
    }
    CredentialSet & operation_type(operation_type_t new_operation_type)
    {
      operation_type_ = new_operation_type;
      return *this;
    }
    CredentialSet & credential_length(uint8_t new_credential_length)
    {
      credential_length_ = new_credential_length;
      return *this;
    }
    CredentialSet & credential_data(std::initializer_list<uint8_t> new_data)
    {
      credential_data_.insert(credential_data_.end(), new_data.begin(), new_data.end());
      return *this;
    }
    operator std::vector<uint8_t>()
    {
      fill();
      return frame_;
    }
private:
    std::vector<uint8_t> frame_;
    uint16_t uuid_;
    credential_type_t credential_type_;
    uint16_t credential_slot_;
    operation_type_t operation_type_;
    uint8_t credential_length_;
    std::vector<uint8_t> credential_data_;

    void fill(void)
    {
      frame_.push_back(COMMAND_CLASS_USER_CREDENTIAL);
      frame_.push_back(CREDENTIAL_SET);
      frame_.push_back(uuid_ >> 8);
      frame_.push_back(uuid_);
      frame_.push_back(static_cast<uint8_t>(credential_type_));
      frame_.push_back(credential_slot_ >> 8);
      frame_.push_back(credential_slot_);
      frame_.push_back(static_cast<uint8_t>(operation_type_));
      if (0 == credential_length_) {
        frame_.push_back(credential_data_.size());
      } else {
        frame_.push_back(credential_length_);
      }
      frame_.insert(frame_.end(), credential_data_.begin(), credential_data_.end());
    }
  };
  class CredentialGet {
public:
    CredentialGet(void)
    {
      frame_.clear();
      uuid_ = 1;
      credential_type_ = credential_type_t::PIN_CODE;
      credential_slot_ = 1;
    }
    CredentialGet & uuid(uint16_t new_uuid)
    {
      uuid_ = new_uuid;
      return *this;
    }
    CredentialGet & credential_type(credential_type_t new_credential_type)
    {
      credential_type_ = new_credential_type;
      return *this;
    }
    CredentialGet & credential_slot(uint16_t new_credential_slot)
    {
      credential_slot_ = new_credential_slot;
      return *this;
    }
    operator std::vector<uint8_t>()
    {
      fill();
      return frame_;
    }
private:
    std::vector<uint8_t> frame_;
    uint16_t uuid_;
    credential_type_t credential_type_;
    uint16_t credential_slot_;

    void fill(void)
    {
      frame_.push_back(COMMAND_CLASS_USER_CREDENTIAL);
      frame_.push_back(CREDENTIAL_GET);
      frame_.push_back(uuid_ >> 8);
      frame_.push_back(uuid_);
      frame_.push_back(static_cast<uint8_t>(credential_type_));
      frame_.push_back(credential_slot_ >> 8);
      frame_.push_back(credential_slot_);
    }
  };
  class UserCredentialAssociationSet {
public:
    UserCredentialAssociationSet(void)
    {
      frame_.clear();
      source_credential_type_ = credential_type_t::PIN_CODE;
      source_credential_slot_ = 1;
      destination_uuid_ = 2;
      destination_credential_slot_ = 1;
    }
    UserCredentialAssociationSet & source_credential_type(credential_type_t new_source_credential_type)
    {
      source_credential_type_ = new_source_credential_type;
      return *this;
    }
    UserCredentialAssociationSet & source_credential_slot(uint16_t new_source_credential_slot)
    {
      source_credential_slot_ = new_source_credential_slot;
      return *this;
    }
    UserCredentialAssociationSet & destination_uuid(uint16_t new_destination_uuid)
    {
      destination_uuid_ = new_destination_uuid;
      return *this;
    }
    UserCredentialAssociationSet & destination_credential_slot(uint16_t new_destination_credential_slot)
    {
      destination_credential_slot_ = new_destination_credential_slot;
      return *this;
    }
    operator std::vector<uint8_t>()
    {
      fill();
      return frame_;
    }
private:
    std::vector<uint8_t> frame_;
    uint16_t destination_uuid_;
    credential_type_t source_credential_type_;
    uint16_t source_credential_slot_;
    uint16_t destination_credential_slot_;

    void fill(void)
    {
      frame_.push_back(COMMAND_CLASS_USER_CREDENTIAL);
      frame_.push_back(USER_CREDENTIAL_ASSOCIATION_SET);
      frame_.push_back(static_cast<uint8_t>(source_credential_type_));
      frame_.push_back(source_credential_slot_ >> 8);
      frame_.push_back(source_credential_slot_);
      frame_.push_back(destination_uuid_ >> 8);
      frame_.push_back(destination_uuid_);
      frame_.push_back(destination_credential_slot_ >> 8);
      frame_.push_back(destination_credential_slot_);
    }
  };
  class AllUsersChecksumGet {
public:
    AllUsersChecksumGet(void)
    {
      frame_.clear();
    }
    operator std::vector<uint8_t>()
    {
      fill();
      return frame_;
    }
private:
    std::vector<uint8_t> frame_;

    void fill(void)
    {
      frame_.push_back(COMMAND_CLASS_USER_CREDENTIAL);
      frame_.push_back(ALL_USERS_CHECKSUM_GET);
    }
  };
  class AllUsersChecksumReport {
public:
    AllUsersChecksumReport(void)
    {
      frame_.clear();
      checksum_ = 0;
    }
    AllUsersChecksumReport& checksum(uint16_t checksum)
    {
      checksum_ = checksum;
      return *this;
    }
    operator std::vector<uint8_t>()
    {
      fill();
      return frame_;
    }
private:
    std::vector<uint8_t> frame_;
    uint16_t checksum_;

    void fill(void)
    {
      frame_.push_back(COMMAND_CLASS_USER_CREDENTIAL);
      frame_.push_back(ALL_USERS_CHECKSUM_REPORT);
      frame_.push_back(checksum_ >> 8);
      frame_.push_back(checksum_);
    }
  };
  class UserChecksumGet {
public:
    UserChecksumGet(void)
    {
      frame_.clear();
      uuid_ = 1;
    }
    UserChecksumGet & uuid(uint16_t new_uuid)
    {
      uuid_ = new_uuid;
      return *this;
    }
    operator std::vector<uint8_t>()
    {
      fill();
      return frame_;
    }
private:
    std::vector<uint8_t> frame_;
    uint16_t uuid_;

    void fill(void)
    {
      frame_.push_back(COMMAND_CLASS_USER_CREDENTIAL);
      frame_.push_back(USER_CHECKSUM_GET);
      frame_.push_back(uuid_ >> 8);
      frame_.push_back(uuid_);
    }
  };
  class UserChecksumReport {
public:
    UserChecksumReport(void)
    {
      frame_.clear();
      uuid_ = 1;
      checksum_ = 0;
    }
    UserChecksumReport& uuid(uint16_t uuid)
    {
      uuid_ = uuid;
      return *this;
    }
    UserChecksumReport& checksum(uint16_t checksum)
    {
      checksum_ = checksum;
      return *this;
    }
    operator std::vector<uint8_t>()
    {
      fill();
      return frame_;
    }
private:
    std::vector<uint8_t> frame_;
    uint16_t uuid_;
    uint16_t checksum_;

    void fill(void)
    {
      frame_.push_back(COMMAND_CLASS_USER_CREDENTIAL);
      frame_.push_back(USER_CHECKSUM_REPORT);
      frame_.push_back(uuid_ >> 8);
      frame_.push_back(uuid_);
      frame_.push_back(checksum_ >> 8);
      frame_.push_back(checksum_);
    }
  };
  class CredentialChecksumGet {
public:
    CredentialChecksumGet(void)
    {
      frame_.clear();
      credential_type_ = 0;
    }
    CredentialChecksumGet & credential_type(uint8_t new_credential_type)
    {
      credential_type_ = new_credential_type;
      return *this;
    }
    operator std::vector<uint8_t>()
    {
      fill();
      return frame_;
    }
private:
    std::vector<uint8_t> frame_;
    uint8_t credential_type_;

    void fill(void)
    {
      frame_.push_back(COMMAND_CLASS_USER_CREDENTIAL);
      frame_.push_back(CREDENTIAL_CHECKSUM_GET);
      frame_.push_back(credential_type_);
    }
  };
  class CredentialChecksumReport {
public:
    CredentialChecksumReport(void)
    {
      frame_.clear();
      credential_type_ = 1;
      checksum_ = 0;
    }
    CredentialChecksumReport& credential_type(uint8_t new_credential_type)
    {
      credential_type_ = new_credential_type;
      return *this;
    }
    CredentialChecksumReport& checksum(uint16_t checksum)
    {
      checksum_ = checksum;
      return *this;
    }
    operator std::vector<uint8_t>()
    {
      fill();
      return frame_;
    }
private:
    std::vector<uint8_t> frame_;
    uint8_t credential_type_;
    uint16_t checksum_;

    void fill(void)
    {
      frame_.push_back(COMMAND_CLASS_USER_CREDENTIAL);
      frame_.push_back(CREDENTIAL_CHECKSUM_REPORT);
      frame_.push_back(credential_type_);
      frame_.push_back(checksum_ >> 8);
      frame_.push_back(checksum_);
    }
  };
};
