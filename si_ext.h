/*******************************************************************************
 * wirbelscan: A plugin for the Video Disk Recorder
 * See the README file for copyright information and how to reach the author.
 ******************************************************************************/
#pragma once

namespace SI_EXT {

/**********************************************************************************************************
*  table   pid           table_id    defined_by
*  PAT     0x00          0x00        MPEG-2  500msec         Program association section
*  CAT     0x01          0x01        MPEG-2  ?               Conditional access section
*  PMT     0x10..0x1FFE  0x02        MPEG-2  500msec         Program Map Table
*  TSDT    0x02          0x03        MPEG-2  10sec           Transport Stream Description Section
***********************************************************************************************************
*  NIT     0x10          0x40        DVB     25msec..10sec   Network Information section(actual)                       (mandatory)
*  SDT     0x11          0x42        DVB     25msec..2sec    Service description section(actual_transport_stream)      (mandatory)
*  EIT     0x12          0x4E        DVB     25msec..2sec    Event Information Section  (actual ts, present/following) (mandatory)
*  TDT     0x14          0x70        DVB     25msec..30sec   Time and date section                                     (mandatory)
***********************************************************************************************************
*  NIT     0x10          0x41        DVB     25msec..10sec   Other Network Information section                         (optional)
*  BAT     0x11          0x4A        DVB     25msec..10sec   Bouquet Association Table                                 (optional)
*  SDT     0x11          0x46        DVB     25msec..10sec   Other Service description section                         (optional)
*  EIT     0x12          0x4F        DVB     25msec..2sec    Event Information Section  (other ts, present/following)  (optional)
*  EIT     0x12          0x50..0x5F  DVB     25msec..10sec   Event Information Section  (actual ts, schedule)          (optional)
*  EIT     0x12          0x60..0x6F  DVB     25msec..10sec   Event Information Section  (other ts,  schedule)          (optional)
*  RST     0x13          0x71        DVB                     Running status section                                    (optional)
*  TOT     0x14          0x73        DVB     25msec..30sec   Time offset section                                       (optional)
*  MIP     0x15
*  ST      0x10..0x14    0x72        DVB     ----            Stuffing section                                          (optional)
*  DIT     0x1E          0x7E        DVB                     Discontinuity Information Section                         (optional)
*  SIT     0x1F          0x7F        DVB                     Selection Information Section                             (optional)
*  UNT     0x10..0x1FFE  0x4B        DVB
*  INT     0x10..0x1FFE  0x4C        DVB                     INT per PMT In PMT: data_broadcast_id = 0x000B
**********************************************************************************************************
*  AIT     0x74                                              AIT Used for MHP
*  DSM-CC  0x10..0x1FFE  0x39        DVB                     DSM-CC addressable section per PMT
*  DSM-CC  0x10..0x1FFE  0x3A        DVB                     DSM-CC, MPE per PMT
*  DSM-CC  0x10..0x1FFE  0x3B        DVB                     DSM-CC, U-N messages, except DDM per PMT
*  DSM-CC  0x10..0x1FFE  0x3C        DVB                     DSM-CC, DDM per PMT
*  DSM-CC  0x10..0x1FFE  0x3D        DVB                     DSM-CC, stream descriptors per PMT
*  DSM-CC  0x10..0x1FFE  0x3E        DVB                     DSM-CC, private data, IP-Datagram per PMT used for DVB-H
*  DSM-CC  0x10..0x1FFE  0x3F        DVB                     DSM-CC addressable section per PMT
**********************************************************************************************************
*          0x10..0x1FFE  0x78                                MPE-FEC Per PMT DVB-H, changed from 0x75
*          0x79                                              RNT
*  CA      0x01          0x80..0x8F  DVB                     CA EMM, CA ECM  (0x80, 0x81 = ECM)
**********************************************************************************************************/

/**************  DESCRIPTORS  ****************************************************************************
 *
 ########################### ISO 13181-1 ########################################################################
 * value    PAT    CAT    PMT    TSDT    NIT    BAT    SDT    EIT    TOT    PMT    SIT    Descriptor_Tag
 **********************************************************************************************************
 * 0x00    Reserved
 * 0x01    Reserved
 **********************************************************************************************************
 * 0x02    *    *    *    *    -    -    -    -    -    -    -    video_stream_descriptor
 * 0x03    *    *    *    *    -    -    -    -    -    -    -    audio_stream_descriptor
 * 0x04    *    *    *    *    -    -    -    -    -    -    -    hierarchy_descriptor
 * 0x05    *    *    *    *    -    -    -    -    -    -    -    registration_descriptor
 * 0x06    *    *    *    *    -    -    -    -    -    -    -    data_stream_alignment_descriptor
 * 0x07    *    *    *    *    -    -    -    -    -    -    -    target_background_grid_descriptor
 * 0x08    *    *    *    *    -    -    -    -    -    -    -    Video_window_descriptor
 * 0x09    *    *    *    *    -    -    -    -    -    -    -    CA_descriptor
 * 0x0A    *    *    *    *    -    -    -    -    -    -    -    ISO_639_language_descriptor
 * 0x0B    *    *    *    *    -    -    -    -    -    -    -    System_clock_descriptor
 * 0x0C    *    *    *    *    -    -    -    -    -    -    -    Multiplex_buffer_utilization_descriptor
 * 0x0D    *    *    *    *    -    -    -    -    -    -    -    Copyright_descriptor
 * 0x0E    *    *    *    *    -    -    -    -    -    -    -    Maximum_bitrate_descriptor
 * 0x0F    *    *    *    *    -    -    -    -    -    -    -    Private_data_indicator_descriptor
 * 0x10    *    *    *    *    -    -    -    -    -    -    -    Smoothing_buffer_descriptor
 * 0x11    *    *    *    *    -    -    -    -    -    -    -    STD_descriptor
 * 0x12    *    *    *    *    -    -    -    -    -    -    -    IBP_descriptor
 **********************************************************************************************************
 * 0x13    -    -    -    -                                CarouselIdentifierDescriptorTag
 * 0x14
 * ...
 * 0x1A    *    *    *    *    -    -    -    -    -    -    -    Defined    in ISO/IEC 13818-6 (DSM-CC)
 **********************************************************************************************************
 * 0x1B    *    *    *    *    -    -    -    -    -    -    -    MPEG-4_video_descriptor
 * 0x1C    *    *    *    *    -    -    -    -    -    -    -    MPEG-4_audio_descriptor
 * 0x1D    *    *    *    *    -    -    -    -    -    -    -    IOD_descriptor
 * 0x1E    *    *    *    *    -    -    -    -    -    -    -    SL_descriptor
 * 0x1F    *    *    *    *    -    -    -    -    -    -    -    FMC_descriptor
 * 0x20    *    *    *    *    -    -    -    -    -    -    -    External_ES_ID_descriptor
 * 0x21    *    *    *    *    -    -    -    -    -    -    -    MuxCode_descriptor
 * 0x22    *    *    *    *    -    -    -    -    -    -    -    FmxBufferSize_descriptor
 * 0x23    *    *    *    *    -    -    -    -    -    -    -    MultiplexBuffer_descriptor
 **********************************************************************************************************
 * 0x24
 * ...
 * 0x3F    ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Reserved
 **********************************************************************************************************
 ########################### EN 300 468 Version 1.8.1 ########################################################################
 * value    PAT    CAT    PMT    TSDT    NIT    BAT    SDT    EIT    TOT    PMT    SIT    Descriptor_Tag
 * 0x40    -    -    -    -    *    -    -    -    -    -    -    network_name_descriptor
 * 0x41    -    -    -    -    *    *    -    -    -    -    -    service_list_descriptor
 * 0x42    -    -    -    -    *    *    *    *    -    -    *    stuffing_descriptor
 * 0x43    -    -    -    -    *    -    -    -    -    -    -    satellite_delivery_system_descriptor
 * 0x44    -    -    -    -    *    -    -    -    -    -    -    cable_delivery_system_descriptor
 * 0x45    -    -    -    -    -    -    -    -    -    *    -    VBI_data_descriptor
 * 0x46    -    -    -    -    -    -    -    -    -    *    -    VBI_teletext_descriptor
 * 0x47    -    -    -    -    -    *    *    -    -    -    *    bouquet_name_descriptor
 * 0x48    -    -    -    -    -    -    *    -    -    -    *    service_descriptor
 * 0x49    -    -    -    -    -    *    *    -    -    -    *    country_availability_descriptor
 * 0x4A    -    -    -    -    *    *    *    *    -    -    *    linkage_descriptor
 * 0x4B    -    -    -    -    -    -    *    -    -    -    *    NVOD_reference_descriptor
 * 0x4C    -    -    -    -    -    -    *    -    -    -    *    time_shifted_service_descriptor
 * 0x4D    -    -    -    -    -    -    -    *    -    -    *    short_event_descriptor
 * 0x4E    -    -    -    -    -    -    -    *    -    -    *    extended_event_descriptor
 * 0x4F    -    -    -    -    -    -    -    *    -    -    *    time_shifted_event_descriptor
 * 0x50    -    -    -    -    -    -    *    *    -    -    *    component_descriptor
 * 0x51    -    -    -    -    -    -    *    -    -    *    *    mosaic_descriptor
 * 0x52    -    -    -    -    -    -    -    -    -    *    -    stream_identifier_descriptor
 * 0x53    -    -    -    -    -    *    *    *    -    -    *    CA_identifier_descriptor
 * 0x54    -    -    -    -    -    -    -    *    -    -    *    content_descriptor
 * 0x55    -    -    -    -    -    -    -    *    -    -    *    parental_rating_descriptor
 * 0x56    -    -    -    -    -    -    -    -    -    *    -    teletext_descriptor
 * 0x57    -    -    -    -    -    -    *    *    -    -    *    telephone_descriptor
 * 0x58    -    -    -    -    -    -    -    -    *    -    -    local_time_offset_descriptor
 * 0x59    -    -    -    -    -    -    -    -    -    *    -    subtitling_descriptor
 * 0x5A    -    -    -    -    *    -    -    -    -    -    -    terrestrial_delivery_system_descriptor
 * 0x5B    -    -    -    -    *    -    -    -    -    -    -    multilingual_network_name_descriptor
 * 0x5C    -    -    -    -    -    *    -    -    -    -    -    multilingual_bouquet_name_descriptor
 * 0x5D    -    -    -    -    -    -    *    -    -    -    *    multilingual_service_name_descriptor
 * 0x5E    -    -    -    -    -    -    -    *    -    -    *    multilingual_component_descriptor
 * 0x5F    -    -    -    -    *    *    *    *    -    *    *    private_data_specifier_descriptor
 * 0x60    -    -    -    -    -    -    -    -    -    *    -    service_move_descriptor
 * 0x61    -    -    -    -    -    -    -    *    -    -    *    short_smoothing_buffer_descriptor
 * 0x62    -    -    -    -    *    -    -    -    -    -    -    frequency_list_descriptor
 * 0x63    -    -    -    -    -    -    -    -    -    -    *    partial_transport_stream_descriptor
 * 0x64    -    -    -    -    -    -    *    *    -    -    *    data_broadcast_descriptor
 * 0x65    -    -    -    -    -    -    -    -    -    *    -    scrambling_descriptor
 * 0x66    -    -    -    -    -    -    -    -    -    *    -    data_broadcast_id_descriptor
 * 0x67    -    -    -    -    -    -    -    -    -    -    -    transport_stream_descriptor
 * 0x68    -    -    -    -    -    -    -    -    -    -    -    DSNG_descriptor
 * 0x69    -    -    -    -    -    -    -    *    -    -    -    PDC_descriptor
 * 0x6A    -    -    -    -    -    -    -    -    -    *    -    AC-3_descriptor
 * 0x6B    -    -    -    -    -    -    -    -    -    *    -    ancillary_data_descriptor
 * 0x6C    -    -    -    -    *    -    -    -    -    -    -    cell_list_descriptor
 * 0x6D    -    -    -    -    *    -    -    -    -    -    -    cell_frequency_link_descriptor
 * 0x6E    -    -    -    -    -    -    *    -    -    -    -    announcement_support_descriptor
 * 0x6F    -    -    -    -    -    -    -    -    -    *    -    application_signalling_descriptor
 * 0x70    -    -    -    -    -    -    -    -    -    *    -    adaptation_field_data_descriptor
 * 0x71    -    -    -    -    -    -    *    -    -    -    -    service_identifier_descriptor
 * 0x72    -    -    -    -    -    -    *    -    -    -    -    service_availability_descriptor
 * 0x73    -    -    -    -    *    *    *    -    -    -    -    default_authority_descriptor
 * 0x74    -    -    -    -    -    -    -    -    -    *    -    related_content_descriptor
 * 0x75    -    -    -    -    -    -    -    *    -    -    -    TVA_id_descriptor
 * 0x76    -    -    -    -    -    -    -    *    -    -    -    content_identifier_descriptor
 * 0x77    -    -    -    -    *    -    -    -    -    -    -    time_slice_fec_identifier_descriptor
 * 0x78    -    -    -    -    -    -    -    -    -    *    -    ECM_repetition_rate_descriptor
 * 0x79    -    -    -    -    *    -    -    -    -    -    -    S2_satellite_delivery_system_descriptor
 * 0x7A    -    -    -    -    -    -    -    -    -    *    -    enhanced_AC-3_descriptor
 * 0x7B    -    -    -    -    -    -    -    -    -    *    -    DTS_descriptor
 * 0x7C    -    -    -    -    -    -    -    -    -    *    -    AAC_descriptor
 * 0x7D    -    -    -    -    -    -    -    -    -    -    -    reserved
 * 0x7E    -    -    -    -    -    -    -    -    -    -    -    reserved
 * 0x7F    -    -    -    -    *    *    *    *    *    *    *    extension_descriptor
 **************************************************************************************************************************************
 * 0x80
 * ..
 * 0xFE   user_defined
 * 0xFF   forbidden
 *
 **********************************************************************************************************/

enum pid_codes {
  PID_PAT                                      = 0x0,
  PID_CAT                                      = 0x1,
  PID_PMT                                      = 0x2,
  PID_TSDT                                     = 0x3,
  PID_NIT                                      = 0x10,
  PID_SDT                                      = 0x11,
  PID_BAT                                      = 0x11,
  PID_EIT                                      = 0x12,
  PID_RST                                      = 0x13,
  PID_TDT                                      = 0x14,
  PID_DIT                                      = 0x1E,
  PID_SIT                                      = 0x1F,
  };
enum table_codes {
  TABLE_ID_PAT                                 = 0x0,  // program allocation table                                  (mandatory)
  TABLE_ID_CAT                                 = 0x1,  // conditional access section                                (mandatory)
  TABLE_ID_PMT                                 = 0x2,  // program map section                                       (mandatory)
  TABLE_ID_TSDT                                = 0x3,  // transport stream description section                      (mandatory)
  TABLE_ID_NIT_ACTUAL                          = 0x40, // network information section, *actual* network             (mandatory)
  TABLE_ID_NIT_OTHER                           = 0x41, // network information section, *other*  network             (optional)
  TABLE_ID_SDT_ACTUAL                          = 0x42, // service description section, *actual* transport stream    (mandatory)
  TABLE_ID_SDT_OTHER                           = 0x46, // service description section, *other*  transport stream    (optional)
  TABLE_ID_BAT                                 = 0x4A, // bouquet association section                               (optional)
  TABLE_ID_EIT_ACTUAL_PRESENT                  = 0x4E, // Event Information Section  (actual ts, present/following) (mandatory)
  TABLE_ID_EIT_OTHER_PRESENT                   = 0x4F, // Event Information Section  (other ts,  present/following) (optional)
  TABLE_ID_EIT_ACTUAL_SCHEDULE_START           = 0x50, // Event Information Section  (actual ts, schedule)          (optional)
  TABLE_ID_EIT_ACTUAL_SCHEDULE_STOP            = 0x5F, // Event Information Section  (actual ts, schedule)          (optional)
  TABLE_ID_EIT_OTHER_SCHEDULE_START            = 0x60, // Event Information Section  (other ts,  schedule)          (optional)
  TABLE_ID_EIT_OTHER_SCHEDULE_STOP             = 0x6F, // Event Information Section  (other ts,  schedule)          (optional)
  TABLE_ID_SIT                                 = 0x7F, // service information section                               (optional)
  TABLE_ID_CIT_PREM                            = 0xA0, // premiere content information section                      (optional, undefined in 13818/300468)
  };
enum _stream_type {
  STREAMTYPE_UNDEFINED                         = 0x0,  // ITU-T | ISO/IEC Reserved
  STREAMTYPE_11172_VIDEO                       = 0x1,  // ISO/IEC 11172 Video
  STREAMTYPE_13818_VIDEO                       = 0x2,  // ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream
  STREAMTYPE_11172_AUDIO                       = 0x3,  // ISO/IEC 11172 Audio
  STREAMTYPE_13818_AUDIO                       = 0x4,  // ISO/IEC 13818-3 Audio
  STREAMTYPE_13818_PRIVATE                     = 0x5,  // ITU-T Rec. H.222.0 | ISO/IEC 13818-1 private_sections
  STREAMTYPE_13818_PES_PRIVATE                 = 0x6,  // ITU-T Rec. H.222.0 | ISO/IEC 13818-1 PES packets containing private data
  STREAMTYPE_13522_MHEG                        = 0x7,  // ISO/IEC 13522 MHEG
  STREAMTYPE_H222_0_DSMCC                      = 0x8,  // ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Annex A DSM-CC
  STREAMTYPE_H222_1                            = 0x9,  // ITU-T Rec. H.222.1 (audiovisual communication in ATM environments)
  STREAMTYPE_13818_6_TYPE_A                    = 0xA,  // ISO/IEC 13818-6 type A (DSM-CC)
  STREAMTYPE_13818_6_TYPE_B                    = 0xB,  // ISO/IEC 13818-6 type B (DSM-CC)
  STREAMTYPE_13818_6_TYPE_C                    = 0xC,  // ISO/IEC 13818-6 type C (DSM-CC)
  STREAMTYPE_13818_6_TYPE_D                    = 0xD,  // ISO/IEC 13818-6 type D (DSM-CC)
  STREAMTYPE_13818_1_AUX                       = 0xE,  // ITU-T Rec. H.222.0 | ISO/IEC 13818-1 auxiliary
  STREAMTYPE_13818_AUDIO_ADTS                  = 0xF,  // ISO/IEC 13818-7 Audio with ADTS transport syntax
  STREAMTYPE_14496_VISUAL                      = 0x10, // ISO/IEC 14496-2 Visual
  STREAMTYPE_14496_AUDIO_LATM                  = 0x11, // ISO/IEC 14496-3 Audio with the LATM transport syntax as defined in ISO/IEC 14496-3 / AMD 1
  STREAMTYPE_14496_FLEX_PES                    = 0x12, // ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in PES packets
  STREAMTYPE_14496_FLEX_SECT                   = 0x13, // ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in ISO/IEC14496_sections.
  STREAMTYPE_13818_6_DOWNLOAD                  = 0x14, // ISO/IEC 13818-6 (DSM-CC) Synchronized Download Protocol
  STREAMTYPE_META_PES                          = 0x15, // Metadata carried in PES packets using the Metadata Access Unit Wrapper defined in 2.12.4.1
  STREAMTYPE_META_SECT                         = 0x16, // Metadata carried in metadata_sections
  STREAMTYPE_DSMCC_DATA                        = 0x17, // Metadata carried in ISO/IEC 13818-6 (DSM-CC) Data Carousel
  STREAMTYPE_DSMCC_OBJ                         = 0x18, // Metadata carried in ISO/IEC 13818-6 (DSM-CC) Object Carousel
  STREAMTYPE_META_DOWNLOAD                     = 0x19, // Metadata carried in ISO/IEC 13818-6 (DSM-CC) Synchronized Download Protocol using the Metadata Access Unit Wrapper defined in 2.12.4.1
  STREAMTYPE_13818_11_IPMP_STREAM              = 0x1A, // IPMP stream (defined in ISO/IEC 13818-11, MPEG-2 IPMP)
  STREAMTYPE_14496_H264_VIDEO                  = 0x1B, // AVC video stream as defined in ITU-T Rec. H.264 | ISO/IEC 14496-10 Video
                                                       // 0x1C-0x7E: ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Reserved
  STREAMTYPE_14496_H264_AUDIO                  = 0x1C, // ISO/IEC 14496-3 Audio, without using any additional transport syntax, such as DST, ALS and SLS
  STREAMTYPE_14496_TEXT                        = 0x1D, // ISO/IEC 14496-17 Text
  STREAMTYPE_23002_AUX_VIDEO                   = 0x1E, // Auxiliary video stream as defined in ISO/IEC 23002-3
  STREAMTYPE_14496_SVC_VIDEO_SUB_BITSTREAM     = 0x1F, // SVC video sub-bitstream of an AVC video stream conforming to one or more profiles defined in
                                                       // Annex G of Rec. ITU-T H.264 | ISO/IEC 14496-10
  STREAMTYPE_14496_MVC_VIDEO_SUB_BITSTREAM     = 0x20, // MVC video sub-bitstream of an AVC video stream conforming to one or more profiles defined in
                                                       // Annex H of Rec. ITU-T H.264 | ISO/IEC 14496-10
  STREAMTYPE_15444_VIDEO                       = 0x21, // JPEG 2000 Video stream conforming to one or more profiles as defined in Rec. ITU-T T.800 | ISO/IEC 15444-1
  STREAMTYPE_13818_3D_ADDITIONAL_VIDEO         = 0x22, // Additional view Rec. ITU-T H.262 | ISO/IEC 13818-2 video stream for service-compatible
                                                       // stereoscopic 3D services (see Notes 3 and 4)
  STREAMTYPE_14496_3D_ADDITIONAL_VIDEO         = 0x23, // Additional view Rec. ITU-T H.264 | ISO/IEC 14496-10 video stream conforming to one or more
                                                       // profiles defined in Annex A for service-compatible stereoscopic 3D services (see Notes 3 and 4)
  STREAMTYPE_23008_H265_VIDEO                  = 0x24, // HEVC video stream or an HEVC temporal video sub-bitstream (ISO/IEC23008-2 / H.265)
  STREAMTYPE_23008_H265_TEMP_VIDEO             = 0x25, // HEVC temporal video subset of an HEVC video stream conforming to one or more profiles defined in
                                                       // Annex A of Rec. ITU-T H.265 | ISO/IEC 23008-2
  STREAMTYPE_14496_MVCD_VIDEO_SUB_BITSTREAM    = 0x26, // ITU-T Rec. H.264 | ISO/IEC 14496-10 Annex I MVCD video sub-bitstream
  STREAMTYPE_IPMP_STREAM                       = 0x7F, // IPMP stream
  STREAMTYPE_13818_USR_PRIVATE_80              = 0x80, // 0x80-0xFF User Private
  STREAMTYPE_13818_USR_PRIVATE_81              = 0x81,
  };
enum DescriptorTag {
  EnhancedAC3DescriptorTag                     = 0x7A,
  DTSDescriptorTag                             = 0x7B,
  AACDescriptorTag                             = 0x7C,
  };
enum stream_content_code {
  MPEG_2_video                                 = 0x1, 
  MPEG_1_Layer_2_audio                         = 0x2, 
  VBI_data                                     = 0x3, 
  AC3_audio_modes                              = 0x4, 
  H264_AVC_video                               = 0x5, 
  HE_AAC_audio                                 = 0x6, 
  DTS_audio                                    = 0x7,
  };
enum service_type_code {
  digital_television_service                   = 0x1,
  digital_radio_sound_service                  = 0x2,
  Teletext_service                             = 0x3,
  digital_television_NVOD_reference_service    = 0x4,
  digital_television_NVOD_timeshifted_service  = 0x5,
  mosaic_service                               = 0x6,
  FM_radio_service                             = 0x7,
  DVB_SRM_service                              = 0x8,
  advanced_codec_digital_radio_sound_service   = 0xA,
  H264_AVC_codec_mosaic_service                = 0xB,
  data_broadcast_service                       = 0xC,
  reserved_Common_Interface_Usage_EN50221      = 0xD,
  RCS_Map_EN301790                             = 0xE,
  RCS_FLS_EN301790                             = 0xF,
  DVB_MHP_service                              = 0x10,
  MPEG2_HD_digital_television_service          = 0x11,
  H264_AVC_SD_digital_television_service       = 0x16,
  H264_AVC_SD_NVOD_timeshifted_service         = 0x17,
  H264_AVC_SD_NVOD_reference_service           = 0x18,
  H264_AVC_HD_digital_television_service       = 0x19,
  H264_AVC_HD_NVOD_timeshifted_service         = 0x1A,
  H264_AVC_HD_NVOD_reference_service           = 0x1B,
  H264_AVC_frame_compat_plano_stereoscopic_HD_digital_television_service = 0x1C,
  H264_AVC_frame_compat_plano_stereoscopic_HD_NVOD_timeshifted_service   = 0x1D,
  H264_AVC_frame_compat_plano_stereoscopic_HD_NVOD_reference_service     = 0x1E,
  HEVC_digital_television_service              = 0x1F,
  };



/* Well, sorting the following #defines is a nightmare.
 * This happens, if some spec allows for *private* data. :-(
 *
 * At least a quarter of those companies was not able to proper enter their details
 * into the dvb application form.
 *
 * Let's try our best to find useful identifiers for them..
 *
 * NOTE: https://en.cppreference.com/w/cpp/language/enum
 */
enum private_data_specifier_codes : std::uint32_t {
   private_data_specifier_Reserved        = 0x00000000, // Reserved
   private_data_specifier_SES             = 0x00000001, // SES
   private_data_specifier_BskyB_1         = 0x00000002, // BskyB 1 - 3
   private_data_specifier_BskyB_2         = 0x00000003, // BskyB 1 - 3
   private_data_specifier_BskyB_3         = 0x00000004, // BskyB 1 - 3
   private_data_specifier_ARD_ZDF_ORF     = 0x00000005, // ARD, ZDF, ORF
   private_data_specifier_Nokia           = 0x00000006, // Nokia Multimedia Network Terminals
   private_data_specifier_AT_Entertain    = 0x00000007, // AT Entertainment Ltd.
   private_data_specifier_TV_Cabo         = 0x00000008, // TV Cabo Portugal
   private_data_specifier_Nagra_1         = 0x00000009, // Nagravision SA – Kudelski 1 - 5
   private_data_specifier_Nagra_2         = 0x0000000A, // Nagravision SA – Kudelski 1 - 5
   private_data_specifier_Nagra_3         = 0x0000000B, // Nagravision SA – Kudelski 1 - 5
   private_data_specifier_Nagra_4         = 0x0000000C, // Nagravision SA – Kudelski 1 - 5
   private_data_specifier_Nagra_5         = 0x0000000D, // Nagravision SA – Kudelski 1 - 5
   private_data_specifier_Valvision       = 0x0000000E, // Valvision SA
   private_data_specifier_Quiero          = 0x0000000F, // Quiero Televisión
   private_data_specifier_TPS_1           = 0x00000010, // La Télévision Par Satellite (TPS)
   private_data_specifier_Echostar        = 0x00000011, // Echostar Communications
   private_data_specifier_Telia           = 0x00000012, // Telia AB
   private_data_specifier_Viasat          = 0x00000013, // Viasat
   private_data_specifier_Boxer_TV        = 0x00000014, // Boxer TV Access
   private_data_specifier_MediaKabel      = 0x00000015, // MediaKabel
   private_data_specifier_Casema          = 0x00000016, // Casema
   private_data_specifier_Humax           = 0x00000017, // Humax Electronics Co. Ltd .
   private_data_specifier_Neotion         = 0x00000018, // Neotion SA
   private_data_specifier_Singapore       = 0x00000019, // Singapore Digital Terrestrial Television
   private_data_specifier_TDF             = 0x0000001A, // Télédiffusion de France (TDF)
   private_data_specifier_Intellibyte     = 0x0000001B, // Intellibyte Inc.
   private_data_specifier_DTS             = 0x0000001C, // Digital Theater Systems Ltd
   private_data_specifier_Finlux          = 0x0000001D, // Finlux Ltd.
   private_data_specifier_Sagem           = 0x0000001E, // Sagem SA
   private_data_specifier_BCTI            = 0x0000001F, // Beijing Compunicate Technology Inc
   private_data_specifier_Lyonnaise_1     = 0x00000020, // Lyonnaise Cable 1 - 4
   private_data_specifier_Lyonnaise_2     = 0x00000021, // Lyonnaise Cable 1 - 4
   private_data_specifier_Lyonnaise_3     = 0x00000022, // Lyonnaise Cable 1 - 4
   private_data_specifier_Lyonnaise_4     = 0x00000023, // Lyonnaise Cable 1 - 4
   private_data_specifier_Metronic        = 0x00000024, // Metronic
   private_data_specifier_MTV             = 0x00000025, // MTV Europe
   private_data_specifier_Pansonic        = 0x00000026, // Pansonic
   private_data_specifier_Mentor          = 0x00000027, // Mentor Data System, Inc .
   private_data_specifier_EACEM           = 0x00000028, // EACEM
   private_data_specifier_NorDig          = 0x00000029, // NorDig
   private_data_specifier_Intelsis        = 0x0000002A, // Intelsis Sistemas Inteligentes S.A .
   private_data_specifier_DTV_HVGY        = 0x0000002B, // DTV haber ve Gorsel yayýncilik
   private_data_specifier_ADS             = 0x0000002D, // Alpha Digital Synthesis S.A.
   private_data_specifier_THOMSON         = 0x0000002E, // THOMSON
   private_data_specifier_Conax           = 0x0000002F, // Conax A.S.
   private_data_specifier_Telenor         = 0x00000030, // Telenor
   private_data_specifier_TeleDenmark     = 0x00000031, // TeleDenmark
   private_data_specifier_Foxtel_1        = 0x00000032, // Foxtel Management
   private_data_specifier_InOutTV_1       = 0x00000033, // InOutTV
   private_data_specifier_InOutTV_2       = 0x00000034, // InOutTV SA (2)
   private_data_specifier_EON             = 0x00000035, // Europe Online Networks S.A .
   private_data_specifier_CanalPlus_1     = 0x00000036, // Groupe Canal+
   private_data_specifier_FreeView        = 0x00000037, // FreeView (NZ)
   private_data_specifier_OTE             = 0x00000038, // OTE
   private_data_specifier_Polsat          = 0x00000039, // Telewizja Polsat
   private_data_specifier_arena           = 0x0000003A, // arena Sport Rechte und Marketing GmbH
   private_data_specifier_Wyplay          = 0x0000003B, // Wyplay SAS
   private_data_specifier_ITAD            = 0x0000003D, // Interactive Technologies AD
   private_data_specifier_TKMK            = 0x0000003E, // T-Kábel Magyarország Kft.
   private_data_specifier_ITI_Neo_1       = 0x0000003F, // ITI Neovision
   private_data_specifier_CiPlus          = 0x00000040, // CI Plus LLP
   private_data_specifier_FTO             = 0x00000041, // France Telecom Orange
   private_data_specifier_CanalPlus_4     = 0x00000046, // Canal+ Luxembourg S.ar.l -- STRANGE. Not clear, if 0x0046xxxx is actually meant
   private_data_specifier_ComHem          = 0x00000050, // Com Hem ab
   private_data_specifier_Sentech         = 0x000000A0, // Sentech
   private_data_specifier_TechniSat       = 0x000000A1, // TechniSat Digital GmbH
   private_data_specifier_LogiWays        = 0x000000A2, // LogiWays
   private_data_specifier_EFG             = 0x000000A3, // EFG
   private_data_specifier_CanalPlus_2     = 0x000000A4, // CANAL+ INTERNATIONAL
   private_data_specifier_CanalPlus_3     = 0x000000A5, // Canal+ Cyfrowy
   private_data_specifier_ITI_Neo_2       = 0x000000B0, // ITI Neovision Sp. z.o.o.
   private_data_specifier_BetaTechnik     = 0x000000BE, // BetaTechnik
   private_data_specifier_NDS             = 0x000000C0, // NDS France
   private_data_specifier_Dolby           = 0x000000D0, // Dolby Laboratories Inc.
   private_data_specifier_ExpressVu       = 0x000000E0, // ExpressVu Inc.
   private_data_specifier_FTCD            = 0x000000F0, // France Telecom, CNES and DGA (STENTOR)
   private_data_specifier_OpenTV_1        = 0x00000100, // OpenTV
   private_data_specifier_MediaBroadcast  = 0x00000110, // Media Broadcast GmbH
   private_data_specifier_Eutelsat_1      = 0x0000013F, // Eutelsat S.A. (1)
   private_data_specifier_Eltrona         = 0x00000140, // Eltrona-Interdiffusion S.A.
   private_data_specifier_Loewe           = 0x00000150, // Loewe Opta GmbH
   private_data_specifier_Triax           = 0x00000160, // Triax A/S
   private_data_specifier_DTAG            = 0x00000170, // Deutsche Telekom AG
   private_data_specifier_EAB             = 0x00000180, // EAB - Ericsson AB
   private_data_specifier_Samsung         = 0x00000190, // Samsung Electronics (UK) Ltd
   private_data_specifier_RCS_RDS         = 0x000001A0, // RCS&RDS
   private_data_specifier_ORS_1           = 0x000001B0, // ORS comm GmbH & Co KG
   private_data_specifier_ORS_2           = 0x000001B1, // Österreichische Rundfunksender GmbH & Co KG
   private_data_specifier_MITxperts       = 0x000001B2, // MIT-xperts GmbH
   private_data_specifier_Eutelsat_2      = 0x0000055F, // Eutelsat S.A. (2)
   private_data_specifier_UPC_1           = 0x00000600, // UPC 1 - 2
   private_data_specifier_UPC_2           = 0x00000601, // UPC 1 - 2
   private_data_specifier_UPC_3           = 0x00000602, // UPC Broadband Holding Services BV
   private_data_specifier_UPC_4           = 0x00000603, // UPC Broadband Holding Services BV
   private_data_specifier_Liberty         = 0x00000604, // Liberty Global Operations B.V.
   private_data_specifier_WISI            = 0x00000A2B, // WISI
   private_data_specifier_Ortikon         = 0x00000ACE, // Ortikon Interactive Oy
   private_data_specifier_Zenterio        = 0x00000AD0, // Zenterio AB
   private_data_specifier_Inview          = 0x00000AF0, // Inview Technology Ltd
   private_data_specifier_SlovakTelecom_1 = 0x00000B00, // Slovak Telecom, a.s
   private_data_specifier_Technicolor     = 0x00000B10, // Technicolor
   private_data_specifier_TPS_2           = 0x00001000, // La Télévision Par Satellite (TPS )
   private_data_specifier_TPSA            = 0x000010F0, // TP S.A.
   private_data_specifier_UCConnect       = 0x00002000, // UC-Connect
   private_data_specifier_ARX             = 0x00002004, // ARX Communications LLC
   private_data_specifier_CRA             = 0x00002046, // Communications Regulatory Agency
   private_data_specifier_Brunei          = 0x00002060, // Radio Televisyen Brunei
   private_data_specifier_Myanma          = 0x00002068, // Myanma Radio and Television
   private_data_specifier_ANAC            = 0x00002084, // ANAC - National Communications Authority
   private_data_specifier_Colombia        = 0x000020AA, // Comision Nacional de Television de Colombia
   private_data_specifier_GNCC            = 0x0000210C, // Georgian National Communications Commission (GNCC)
   private_data_specifier_EETT            = 0x0000212C, // EETT
   private_data_specifier_Vodafone        = 0x00002160, // Vodafone Iceland
   private_data_specifier_Indonesia       = 0x00002168, // Ministry of Communication and Information Technology of the Republic of Indonesia
   private_data_specifier_HACA            = 0x00002180, // HACA
   private_data_specifier_MYTV            = 0x000021CA, // MYTV
   private_data_specifier_CE              = 0x000021EC, // Communications électroniques
   private_data_specifier_BTP             = 0x00002213, // Bureau Telecommunicatie en Post
   private_data_specifier_ANSP            = 0x0000224F, // Autorida Nacional de los Servicios Publicos
   private_data_specifier_NICTA           = 0x00002256, // NICTA
   private_data_specifier_OEC             = 0x00002268, // Office of Electronic Communications
   private_data_specifier_RTRN            = 0x00002283, // RTRN
   private_data_specifier_JPETIV          = 0x000022B0, // JP Emisiona Tehnika i Veze
   private_data_specifier_Seychelles      = 0x000022B2, // Seychelles Broadcasting Corporation
   private_data_specifier_SlovakTelecom_2 = 0x000022BF, // Telecommunications office of the Slovak republic
   private_data_specifier_Slovenia        = 0x000022C1, // DTT - Slovenia Digital Terrestrial Television
   private_data_specifier_SouthAfrica     = 0x000022C6, // DTT - South African Digital Terrestrial Television
   private_data_specifier_Hungarian       = 0x000022C7, // DTT- Hungarian Digital Terrestrial Television
   private_data_specifier_SIALattelecom   = 0x000022C8, // SIA Lattelecom
   private_data_specifier_IrishDTT        = 0x000022CE, // DTT - Irish Digital Terrestrial Television
   private_data_specifier_PortugalDTT     = 0x000022CF, // DTT- Portugal Digital Terrestrial Television
   private_data_specifier_Spain           = 0x000022D4, // Spanish Broadcasting Regulator
   private_data_specifier_MoICT           = 0x000022EC, // Ministry of ICT
   private_data_specifier_Sweden          = 0x000022F1, // Swedish Broadcasting Regulator
   private_data_specifier_NBTC            = 0x000022FC, // Office of National Broadcasting and Telecommunications Commission
   private_data_specifier_TRA_UAE         = 0x00002310, // Telecommunications Regulatory Authority (TRA) UAE
   private_data_specifier_Uganda          = 0x00002320, // Uganda Commuications Commission
   private_data_specifier_ITC             = 0x0000233A, // Independent Television Commission
   private_data_specifier_OotR            = 0x00002372, // Office of the Regulator
   private_data_specifier_Sky             = 0x00002B00, // Sky Network Television Limited
   private_data_specifier_Australian      = 0x00003200, // Australian Terrestrial Television Networks
   private_data_specifier_DigiTV          = 0x0000333A, // Digital TV Group
   private_data_specifier_NewsDatacom     = 0x00006000, // News Datacom
   private_data_specifier_NDC_1           = 0x00006001, // NDC 1 - 6
   private_data_specifier_NDC_2           = 0x00006002, // NDC 1 - 6
   private_data_specifier_NDC_3           = 0x00006003, // NDC 1 - 6
   private_data_specifier_NDC_4           = 0x00006004, // NDC 1 - 6
   private_data_specifier_NDC_5           = 0x00006005, // NDC 1 - 6
   private_data_specifier_NDC_6           = 0x00006006, // NDC 1 - 6
   private_data_specifier_Irdeto          = 0x00362275, // Irdeto
   private_data_specifier_NTL             = 0x004E544C, // NTL
   private_data_specifier_SciAtlanta_1    = 0x00532D41, // Scientific Atlanta
   private_data_specifier_VOO             = 0x00564F4F, // VOO (Tecteo)
   private_data_specifier_RhoneVision     = 0x00600000, // Rhône Vision Cable
   private_data_specifier_OpenMedia       = 0x414F4D53, // Alliance for Open Media
   private_data_specifier_Foxtel_2        = 0x41555300, // Foxtel Management
   private_data_specifier_China           = 0x41565356, // Audio Video Coding Standard Workgroup of China
   private_data_specifier_NCDIL_1         = 0x44414E59, // News Datacom (IL) 1
   private_data_specifier_NCDIL_2         = 0x46524549, // News Datacom (IL) 1
   private_data_specifier_BBC_1           = 0x46534154, // BBC
   private_data_specifier_BBC_2           = 0x46536174, // BBC
   private_data_specifier_FreeTV          = 0x46545600, // FreeTV 1 - 33
   private_data_specifier_FOXTEL_1        = 0x46585431, // FOXTEL Management Pty Ltd
   private_data_specifier_FOXTEL_2        = 0x4658544C, // FOXTEL Management Pty Ltd
   private_data_specifier_KabelDeutschland= 0x4A4F4A4F, // Kabel Deutschland
   private_data_specifier_OpenTV_4F_00    = 0x4F545600, // OpenTV 1 - 256
   private_data_specifier_OpenTV_4F_FF    = 0x4F5456FF, // OpenTV 1 - 256
   private_data_specifier_Philips_50_00   = 0x50484900, // Philips DVS 1 - 256
   private_data_specifier_Philips_50_FF   = 0x50484900, // Philips DVS 1 - 256
   private_data_specifier_SciAtlanta_2    = 0x53415053, // Scientific Atlanta
   private_data_specifier_StarGuide       = 0x5347444E, // StarGuide Digital Networks
   private_data_specifier_GkWare          = 0x53475255, // GkWare e.K.
   private_data_specifier_Via             = 0x56444700, // Vía Digital
   private_data_specifier_VideoLAN        = 0x564C414e, // VideoLAN
   private_data_specifier_TechnoTrend     = 0x564F564F, // TechnoTrend AG
   private_data_specifier_Bertelsmann     = 0xBBBBBBBB, // Bertelsmann Broadband Group
   private_data_specifier_eventIS         = 0xE0E0E0E0, // eventIS
   private_data_specifier_ECCA            = 0xECCA0001, // ECCA (European Cable Communications Association)
   private_data_specifier_FTC             = 0xFCFCFCFC, // France Telecom
};

/************* do not touch ***************/
} //end of namespace
