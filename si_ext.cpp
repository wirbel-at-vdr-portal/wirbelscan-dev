/*******************************************************************************
 * wirbelscan: A plugin for the Video Disk Recorder
 * See the README file for copyright information and how to reach the author.
 ******************************************************************************/
#include <cstdint> // std::uint32_t
#include <endian.h>
#include "scanfilter.h"
#include "si_ext.h"


struct descr_generic {
  u_char descriptor_tag                          :8;
  u_char descriptor_length                       :8;
};

struct item_logical_channel {
  u_char service_id_hi                           :8;
  u_char service_id_lo                           :8;
  #if BYTE_ORDER == BIG_ENDIAN
  u_char visible_service_flag                    :1;
  u_char logical_channel_number_hi               :7;
  #else
  u_char logical_channel_number_hi               :7;
  u_char visible_service_flag                    :1;
  #endif
  u_char logical_channel_number_lo               :8;
};


/******************************************************************************/
namespace SI_EACEM {

/*******************************************************************************
 * SI_EACEM::LogicalChannelDescriptor, IEC/CENELEC 62216
 ******************************************************************************/

/* SYNTAX:
 * logical_channel_descriptor () {
 *    descriptor_tag             8 uimsbf, 0x83 (HdSimulcast: 0x88)
 *    descriptor_length          8 uimsbf
 *    for(i=0;i<N;i++) {
 *       service_id             16 uimsbf
 *       visible_service_flag    1 Bslbf
 *       reserved                5
 *       logical_channel_number 10 uimsbf
 *       }
 */

void LogicalChannelDescriptor::Parse() {
  const descr_generic* head = data.getData<const descr_generic>();
  LogicalChannels.setData(data+sizeof(descr_generic), getLength()-sizeof(descr_generic));
  LogicalChannel lc;
  for(SI::Loop::Iterator it; LogicalChannels.getNext(lc, it);) {
     lc.HdSimulcast = (head->descriptor_tag == HdSimulcastLogicalChannelDescriptorTag);
     }
}

int LogicalChannel::ServiceId() const {
  return HILO(s->service_id);
}

int LogicalChannel::LCN() const {
  return ((uint16_t) HILO(s->logical_channel_number) ) & 0x3FF;
}

bool LogicalChannel::Visible() const {
  return s->visible_service_flag > 0;
}

int LogicalChannel::getLength() {
  return sizeof(item_logical_channel);
}

void LogicalChannel::Parse() {
  s = data.getData<const item_logical_channel>();
}

} // end of namespace SI_EACEM


/******************************************************************************/
namespace SI_NORDIG {

/*******************************************************************************
 * SI_NORDIG::LogicalChannelDescriptor, NorDig Unified Requirements v3.2
 ******************************************************************************/

/* SYNTAX:
 * logical_channel_descriptor () {
 *    descriptor_tag             8 uimsbf, 0x83
 *    descriptor_length          8 uimsbf
 *    for(i=0;i<N;i++) {
 *       service_id             16 uimsbf
 *       visible_service_flag    1 Bslbf
 *       reserved                1
 *       logical_channel_number 14 uimsbf
 *       }
 */

int LogicalChannel::LCN() const {
  return ((uint16_t) HILO(s->logical_channel_number) ) & 0x3FFF;
}

/*******************************************************************************
 * SI_NORDIG::LogicalChannelDescriptor (Version 2)
 ******************************************************************************/

/* The older version of the NorDig Logical Channel Descriptor is in some NorDig Networks
 * replaced by the newer version 2 below.
 * The NorDig IRD shall at least store the sorting from one of the available Channel lists
 * as default, but it is recommended that the NorDig IRD store all the transmitted
 * Channel Lists sorting that matches the IRD’s country code settings (especially for
 * IRDs that are not letting the user choose list during installation).
 * When several Channel Lists are available from same network (original network id) for
 * the IRD during first time installation (or complete re-installation), the NorDig IRD
 * shall choose the channel list as the default one with following priority:
 *
 *    1. The list with same country code as the IRD’s user preference setting’s
 *       country code. If several lists available with same matching country code, the
 *       IRD shall choose the one with lowest list_id value OR let the viewer choose
 *       from a list, (typically using the channel_list_name).
 *
 *    2. If no Channel list has a country code that matches the user preference
 *       setting’s country code, the NorDig IRD shall let the viewer choose from a
 *       list (recommended) OR choose the one with lowest list_id value.
 *
 * When broadcasting both LCD version 1 and version 2 within one Original Network ID,
 * the NorDig IRD supporting both descriptors *shall* only sort according to the
 * version 2 (i.e. NorDig LCD version 2 has higher priority).
 */

/* SYNTAX:
 * logical_channel_descriptor () {
 *    descriptor_tag                8 uimsbf, 0x87
 *    descriptor_length             8 uimsbf
 *    for(i=0;i<N;i++) {
 *       channel_list_id            8 uimbsf
 *       channel_list_name_length   8 uimbsf
 *       for(j=0;j<N;j++) {
 *          char                    8 uimbsf, channel_list_name, encoded as 300468
 *          }
 *       country_code              24 uimbsf, see countries.h: std::string Alpha3(void);
 *       descriptor_length          8 uimbsf
 *       for(k=0;k<number_of_services;k++) {
 *          service_id             16 uimbsf
 *          visible_service_flag    1 bslbf
 *          reserved_future_use     5
 *          logical_channel_number 10 uimbsf
 *          }
 *       }
 *    }
 */

void LogicalChannelDescriptorV2::Parse() {
  Loop.setData(data+sizeof(descr_generic), getLength()-sizeof(descr_generic));
  //Loop.Parse();
}

void LogicalChannelList::Parse() {
dlog(5, std::string(__PRETTY_FUNCTION__));
  struct desc_part1 {
    u_char channel_list_id          :8;
    u_char channel_list_name_length :8;
  };
  
  struct desc_part2 {
    u_char country_code_0           :8;
    u_char country_code_1           :8;
    u_char country_code_2           :8;
    u_char descriptor_length        :8;
  };

  const desc_part1* part1 = data.getData<const desc_part1>();
  data.addOffset(sizeof(desc_part1));
  ListId = part1->channel_list_id;

  { /* get list name with correct character encoding. */
  SI::String str;
  str.setData(data, part1->channel_list_name_length);
  const char* cstr = str.getText();
  Name = cstr;
  data.addOffset(part1->channel_list_name_length);
  delete cstr;
  }

  const desc_part2* part2 = data.getData<const desc_part2>();
  CountryCode = std::string(' ',3);
  CountryCode[0] = part2->country_code_0;
  CountryCode[1] = part2->country_code_1;
  CountryCode[2] = part2->country_code_2;
  data.addOffset(sizeof(desc_part2));

  length = 6 + part1->channel_list_name_length + part2->descriptor_length;

  LogicalChannels.setData(data,
                          getLength() -
                          sizeof(desc_part1) -
                          part1->channel_list_name_length -
                          sizeof(desc_part2));
}

int LogicalChannelList::getLength() {
  dlog(5, "length = " + IntToStr(length));
  return length;
}

} // end of namespace SI_NORDIG
