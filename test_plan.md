This defines the tests that need to be run on the Active Schedule CC implementation baked into the Door Lock Key Pad sample application

- [ ] Active Schedule Capabilites Get
    [ ] Check that reported values match the compiled configs
    [ ] Run another build with Year Day and Daily Repeating Schedules per user set to 0, ensure that Active Schedule CC Capabilities do not list 0x83 as a supported command class. (CC:00A4.01.02.11.007)
    [ ] Verify that the User Capabilities Report and the AS Capabilities Report list the same number of Users/Target IDS (CC:00A4.01.02.11.004)

**Unless otherwise specified, all of the following commands will require Target validation consisting of the following steps:**

Target Validation: 
 - Ignore if Target CC = 0
 - Ignore if Target CC is not 0x83 (User Credential Command Class)
 - Ignore if Slot ID is = 0
 - Ignore if Slot ID > # of Users 
 - Ignore if User doesn't exist
 - Ignore if User is of type Expiring  

- [ ] Active Schedule Enable Set
  - [ ] **Target Validation** tests 
  - [ ] Check that report is sent in the following scenarios:
    - [ ] Schedule for valid user is enabled
      - [ ] Test sample sourced enabling
      - [ ] Test Z-wave sourced enabling
    - [ ] Schedule for valid user is disabled         
      - [ ] Test sample sourced disabling
      - [ ] Test Z-Wave sourced disabling

- [ ] Active Schedule Enable Get 
  - [ ] Test Target Validation

- [ ] Active Schedule Set  
  - [ ] Year Day
    - [ ] **Target Validation** tests
    - [ ] Invalid set action is ignored
    - [ ] Invalid Schedule Slot is ignored (0, > YD slots)
    - [ ] Modify 
      - [ ] Time fence invalid is ignored 
        - [ ] Invalid m/y/d/h/m
        - [ ] Valid time stamps, but start occurs after finish
      - [ ]  Valid set
    - [ ] Clear
      - [ ] Check 0 slot number edge case
        - [ ] Verify that all YD schedules are deleted for a given user 
      - [ ] Verify that delete functionality works
  - [ ] Daily Repeating
    - [ ] **Target Validation** tests
    - [ ] Invalid set action is ignored
    - [ ] Invalid Schedule Slot is ignored (0, > DR slots)
    - [ ] Modify 
      - [ ] Zero duration is ignored 
      - [ ] No selected days is ignored
    - [ ] Clear
      - [ ] Check 0 slot number edge case
        - [ ] Verify that all DR schedules are deleted for a given user 
      - [ ] Verify that delete functionality works
  - [ ] Modify a schedule from outside of Z-Wave and make sure that the proper report is sent

- [ ] Active Schedule Get
  - [ ] **Target Validation** tests
  - [ ] Invalid Schedule Slot is ignored

- [ ] Various U3C Integrations
  - [ ] Test that schedules for a User are deleted when that User is converted to Expiring User
  - [ ] Test that schedules for a User are deleted when that User is deleted
  - [ ] Test that schedules for a User are deleted 