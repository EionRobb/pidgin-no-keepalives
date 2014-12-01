#define PURPLE_PLUGINS

#include <glib.h>
#include <plugin.h>
#include <debug.h>
#include <account.h>
#include <accountopt.h>

#ifndef _
#	define _(a) a
#endif

#define NO_KEEPALIVES_ID "eionrobb_no_keepalives"
static GHashTable *original_keepalives = NULL;

void (*keepalive_func)(PurpleConnection *gc);

static void
new_keepalive(PurpleConnection *gc)
{
	void(* keepalive_func)(PurpleConnection *gc);
	const char *prpl_id;
	PurpleAccount *account = gc->account;
	
	if (account == NULL)
	{
		purple_debug_error("no-keepalives", "NULL account\n");
		return;
	}
	
	if (!purple_account_get_bool(account, NO_KEEPALIVES_ID, TRUE))
		return;
	
	prpl_id = purple_plugin_get_id(gc->prpl);
	keepalive_func = g_hash_table_lookup(original_keepalives, prpl_id);
	if (keepalive_func)
		return keepalive_func(gc);
	
	purple_debug_error("no-keepalives", "No original keepalive func found\n");
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	GList *proto_list;
	PurplePlugin *proto;
	PurpleAccountOption *option;
	PurplePluginProtocolInfo *prpl_info;
	
	//Hook into every protocol's prpl_info->keepalive function
	original_keepalives = g_hash_table_new(g_str_hash, g_str_equal);
	
	proto_list = purple_plugins_get_protocols();
	
	for(; proto_list; proto_list = proto_list->next)
	{
		proto = proto_list->data;
		prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(proto);
		
		if (prpl_info->keepalive == NULL) continue;
		
		g_hash_table_insert(original_keepalives, (gpointer)purple_plugin_get_id(proto), prpl_info->keepalive);
		prpl_info->keepalive = new_keepalive;

		//Add in some protocol preferences
		option = purple_account_option_bool_new(_("Enable Keepalives"), NO_KEEPALIVES_ID, TRUE);
		prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
	}
	
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	//Unhook
	GList *proto_list;
	gpointer func;
	GList *list, *llist;
	PurplePluginProtocolInfo *prpl_info;
	PurpleAccountOption *option;
	PurplePlugin *proto;
	const gchar *setting;
	
	proto_list = purple_plugins_get_protocols();
	for(; proto_list; proto_list = proto_list->next)
	{
		proto = (PurplePlugin *) proto_list->data;
		prpl_info = PURPLE_PLUGIN_PROTOCOL_INFO(proto);
		func = g_hash_table_lookup(original_keepalives, purple_plugin_get_id(proto));
		if (func)
		{
			prpl_info->keepalive = func;
			g_hash_table_remove(original_keepalives, purple_plugin_get_id(proto));
		}
		
		//Remove from the protocol options screen
		list = prpl_info->protocol_options;
		while(list != NULL)
		{
			option = (PurpleAccountOption *) list->data;
			setting = purple_account_option_get_setting(option);
			if (setting && g_str_equal(setting, NO_KEEPALIVES_ID))
			{
				llist = list;
				if (llist->prev)
					llist->prev->next = llist->next;
				if (llist->next)
					llist->next->prev = llist->prev;
				
				purple_account_option_destroy(option);
				
				list = g_list_next(list);
				g_list_free_1(llist);
			} else {
				list = g_list_next(list);
			}
		}
	}
	g_hash_table_destroy(original_keepalives);
	original_keepalives = NULL;
	
	return TRUE;
}

static void
plugin_init(PurplePlugin *plugin)
{
}

static PurplePluginInfo info = 
{
	PURPLE_PLUGIN_MAGIC,
	2,
	4,
	PURPLE_PLUGIN_STANDARD,
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_LOWEST,

	NO_KEEPALIVES_ID,
	"No Keepalives",
	"0.1",
	"Disables keepalives per account",
	"Helps with buggy servers or buggy protocols",
	"Eion Robb <eionrobb@gmail.com>",
	"", //URL
	
	plugin_load,
	plugin_unload,
	NULL,
	
	NULL,
	NULL,
	NULL,
	NULL,
	
	NULL,
	NULL,
	NULL,
	NULL
};

PURPLE_INIT_PLUGIN(no_keepalives, plugin_init, info);
