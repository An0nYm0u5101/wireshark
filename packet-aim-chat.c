/* packet-aim-chat.c
 * Routines for AIM Instant Messenger (OSCAR) dissection, SNAC Chat
 * Copyright 2004, Jelmer Vernooij <jelmer@samba.org>
 * Copyright 2000, Ralf Hoelzer <ralf@well.com>
 *
 * $Id: packet-aim-chat.c,v 1.2 2004/03/23 18:36:05 guy Exp $
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <glib.h>

#include <epan/packet.h>
#include <epan/strutil.h>

#include "packet-tcp.h"
#include "packet-aim.h"
#include "prefs.h"

/* SNAC families */
#define FAMILY_CHAT       0x000E

/* Family Chat */
#define FAMILY_CHAT_ERROR             0x0001
#define FAMILY_CHAT_ROOMINFOUPDATE    0x0002
#define FAMILY_CHAT_USERJOIN          0x0003
#define FAMILY_CHAT_USERLEAVE         0x0004
#define FAMILY_CHAT_OUTGOINGMSG       0x0005
#define FAMILY_CHAT_INCOMINGMSG       0x0006
#define FAMILY_CHAT_EVIL_REQ          0x0007
#define FAMILY_CHAT_EVIL_REPLY        0x0008
#define FAMILY_CHAT_DEFAULT           0xffff

static const value_string aim_fnac_family_chat[] = {
  { FAMILY_CHAT_ERROR, "Error" },
  { FAMILY_CHAT_ROOMINFOUPDATE, "Room Info Update" },
  { FAMILY_CHAT_USERJOIN, "User Join" },
  { FAMILY_CHAT_USERLEAVE, "User Leave" },
  { FAMILY_CHAT_OUTGOINGMSG, "Outgoing Message" },
  { FAMILY_CHAT_INCOMINGMSG, "Incoming Message" },
  { FAMILY_CHAT_EVIL_REQ, "Evil Request" },
  { FAMILY_CHAT_EVIL_REPLY, "Evil Reply" },
  { FAMILY_CHAT_DEFAULT, "Chat Default" },
  { 0, NULL }
};

/* Initialize the protocol and registered fields */
static int proto_aim_chat = -1;

/* Initialize the subtree pointers */
static gint ett_aim_chat          = -1;

static int dissect_aim_snac_chat(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  guint8 buddyname_length = 0;
  int offset = 0;
  struct aiminfo *aiminfo = pinfo->private_data;
  char buddyname[MAX_BUDDYNAME_LENGTH + 1];
  guchar msg[1000];

  switch(aiminfo->subtype)
    {
	case FAMILY_CHAT_ERROR:
      return dissect_aim_snac_error(tvb, pinfo, offset, tree);
	case FAMILY_CHAT_ROOMINFOUPDATE:
	case FAMILY_CHAT_USERJOIN:
	case FAMILY_CHAT_USERLEAVE:
	case FAMILY_CHAT_EVIL_REQ:
	case FAMILY_CHAT_EVIL_REPLY:
	  /* FIXME */
	  return 0;
    case FAMILY_CHAT_OUTGOINGMSG:
      /* channel message from client */
      aim_get_message( msg, tvb, 40 + buddyname_length, tvb_length(tvb) 
		   - 40 - buddyname_length );
      
      if (check_col(pinfo->cinfo, COL_INFO)) 
	col_append_fstr(pinfo->cinfo, COL_INFO, " -> %s", msg);
	  return tvb_length(tvb);
      
    case FAMILY_CHAT_INCOMINGMSG:
      /* channel message to client */
      buddyname_length = aim_get_buddyname( buddyname, tvb, 30, 31 );
      aim_get_message( msg, tvb, 36 + buddyname_length, tvb_length(tvb) 
		   - 36 - buddyname_length );
      
      if (check_col(pinfo->cinfo, COL_INFO)) {
	col_append_fstr(pinfo->cinfo, COL_INFO, "from: %s", buddyname);
	col_append_fstr(pinfo->cinfo, COL_INFO, " -> %s", msg);
      }
      
      if(tree) {
	proto_tree_add_text(tree, tvb, 31, buddyname_length, 
			    "Screen Name: %s", buddyname);
      }
	  return tvb_length(tvb);
	default: return 0;
    }
}

/* Register the protocol with Ethereal */
void
proto_register_aim_chat(void)
{

/* Setup list of header fields */
/*FIXME
  static hf_register_info hf[] = {
  };*/

/* Setup protocol subtree array */
  static gint *ett[] = {
    &ett_aim_chat,
  };

/* Register the protocol name and description */
  proto_aim_chat = proto_register_protocol("AIM Chat Service", "AIM Chat", "aim_chat");

/* Required function calls to register the header fields and subtrees used */
/*FIXME
  proto_register_field_array(proto_aim_chat, hf, array_length(hf));*/
  proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_aim_chat(void)
{
  dissector_handle_t aim_handle;
  aim_handle = new_create_dissector_handle(dissect_aim_snac_chat, proto_aim_chat);
  dissector_add("aim.family", FAMILY_CHAT, aim_handle);
  aim_init_family(FAMILY_CHAT, "Chat", aim_fnac_family_chat);
}
