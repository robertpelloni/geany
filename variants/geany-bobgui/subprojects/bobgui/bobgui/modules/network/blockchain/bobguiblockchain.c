#include "bobguiblockchain.h"
G_DEFINE_TYPE (BobguiBlockchainManager, bobgui_blockchain_manager, G_TYPE_OBJECT)
static void bobgui_blockchain_manager_init (BobguiBlockchainManager *s) {}
static void bobgui_blockchain_manager_class_init (BobguiBlockchainManagerClass *k) {}
BobguiBlockchainManager * bobgui_blockchain_manager_get_default (void) {
    static BobguiBlockchainManager *m = NULL;
    if (!m) m = g_object_new (BOBGUI_TYPE_BLOCKCHAIN_MANAGER, NULL);
    return m;
}
