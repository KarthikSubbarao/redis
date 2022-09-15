#include "redismodule.h"

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

/* This is a second sample module to validate that custom authentication callbacks can be registered
 * from multiple modules. */

/* Non Blocking Custom Auth callback / implementation. */
int auth_cb(RedisModuleCtx *ctx, RedisModuleString *username, RedisModuleString *password, const char **err) {
    const char* user = RedisModule_StringPtrLen(username, NULL);
    const char* pwd = RedisModule_StringPtrLen(password, NULL);
    if (!strcmp(user,"foo") && !strcmp(pwd,"allow_two")) {
        RedisModule_AuthenticateClientWithACLUser(ctx, "foo", 3, NULL, NULL, NULL);
        return REDISMODULE_AUTH_SUCCEEDED;
    }
    else if (!strcmp(user,"foo") && !strcmp(pwd,"deny_two")) {
        RedisModuleUser *user = RedisModule_GetModuleUserFromUserName(username);
        if (user) {
            RedisModule_ACLAddLogEntry(ctx, user, NULL, REDISMODULE_ACL_LOG_AUTH);
            RedisModule_FreeModuleUser(user);
        }
        *err = "Auth denied by Misc Module.";
        return REDISMODULE_AUTH_DENIED;
    }
    return REDISMODULE_AUTH_NOT_HANDLED;
}

int test_rm_register_auth_cb(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);
    RedisModule_RegisterCustomAuthCallback(ctx, auth_cb);
    RedisModule_ReplyWithSimpleString(ctx, "OK");
    return REDISMODULE_OK;
}

int test_rm_unregister_auth_cbs(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);
    if (RedisModule_UnregisterCustomAuthCallbacks(ctx) == REDISMODULE_OK) {
        RedisModule_ReplyWithSimpleString(ctx, "OK");
        return REDISMODULE_OK;
    }
    else if (errno == ENOENT) {
        RedisModule_ReplyWithError(ctx, "ERR - no custom auth cbs registered by this module");
    }
    else if (errno == EINPROGRESS) {
        RedisModule_ReplyWithError(ctx, "ERR - cannot unregister cbs as custom auth is in progress");
    }
    return REDISMODULE_ERR;
}

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);
    if (RedisModule_Init(ctx,"customauthtwo",1,REDISMODULE_APIVER_1)== REDISMODULE_ERR)
        return REDISMODULE_ERR;
    if (RedisModule_CreateCommand(ctx,"testmoduletwo.rm_register_auth_cb", test_rm_register_auth_cb,"",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;
    if (RedisModule_CreateCommand(ctx,"testmoduletwo.rm_unregister_auth_cbs", test_rm_unregister_auth_cbs,"",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;
    return REDISMODULE_OK;
}