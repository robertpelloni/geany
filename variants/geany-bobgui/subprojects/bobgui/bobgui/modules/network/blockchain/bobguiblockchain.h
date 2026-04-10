/* bobgui/modules/network/blockchain/bobguiblockchain.h */
#ifndef BOBGUI_BLOCKCHAIN_H
#define BOBGUI_BLOCKCHAIN_H

#include <glib-object.h>

G_BEGIN_DECLS

/* Web3 & Decentralized Hub (Better than custom 3rd party integrations) */
#define BOBGUI_TYPE_BLOCKCHAIN_MANAGER (bobgui_blockchain_manager_get_type ())
G_DECLARE_FINAL_TYPE (BobguiBlockchainManager, bobgui_blockchain_manager, BOBGUI, BLOCKCHAIN_MANAGER, GObject)

BobguiBlockchainManager * bobgui_blockchain_manager_get_default (void);

/* Wallet and Identity (Integrated Secure Memory) */
void bobgui_blockchain_connect_wallet (BobguiBlockchainManager *self, const char *provider);
void bobgui_blockchain_sign_message (BobguiBlockchainManager *self, 
                                    const char *message, 
                                    GAsyncReadyCallback callback);

/* Contract Interaction (Reactive Binding to Smart Contracts) */
void bobgui_blockchain_call_contract (BobguiBlockchainManager *self, 
                                     const char *address, 
                                     const char *abi_json, 
                                     const char *method);

/* Decentralized Storage (IPFS Integration) */
void bobgui_blockchain_upload_ipfs (BobguiBlockchainManager *self, 
                                   GBytes *data, 
                                   GAsyncReadyCallback callback);

G_END_DECLS

#endif /* BOBGUI_BLOCKCHAIN_H */
