/** @file       application.c
 *  @brief      cwASIO test application
 *  @author     Stefan Heinzmann
 *  @version    1.0
 *  @date       2023-2025
 *  @copyright  See file LICENSE in toplevel directory
 * @addtogroup cwASIO_test
 *  @{
 */

#include "cwASIO.h"
#include <malloc.h>
#include <stdio.h>
#include <string.h>

struct Context {
    char *name;
    char *id;
    char *descr;
};

static bool callback(void *ctx, char const *name, char const *id, char const *descr) {
    struct Context *c = ctx;
    if(c && !c->id && strcmp(c->name, name) == 0) {
        c->id = id ? strdup(id) : NULL;
        c->descr = descr ? strdup(descr) : NULL;
    }
    if(!c)
        printf("%s (%s): %s\n", name, id ? id : "", descr ? descr : "");
    return true;
}

int main(int argc, char *argv[]) {
    struct Context ctx = {argc > 1 ? argv[1] : "", NULL, NULL};

    if(strlen(ctx.name) == 0) {
        printf("No device name given. List of available devices:\n");
        return cwASIOenumerate(&callback, NULL);
    }

    cwASIOenumerate(&callback, &ctx);
    if(!ctx.id) {
        printf("Device %s not found.\n", ctx.name);
        return 1;
    }

    printf("Instantiating device %s (%s): %s\n", ctx.name, ctx.id, ctx.descr ? ctx.descr : "");
    struct cwASIODriver *drv = NULL;
    long err = cwASIOload(ctx.id, &drv);
    if(err != 0) {
        printf("Couldn't instantiate %s! Error: %ld\n", ctx.name, err);
        goto cleanup;
    }
    
    err = drv->lpVtbl->future(drv, kcwASIOsetInstanceName, ctx.name);
    switch(err) {
        case ASE_SUCCESS:
            printf("Driver for %s supports setting instance name.\n", ctx.name);
            break;
        case ASE_NotPresent:
            printf("Driver for %s can't find its settings.\n", ctx.name);
            break;
        case ASE_InvalidParameter:
            printf("Driver for %s doesn't support setting instance name.\n", ctx.name);
            break;
        default:
            printf("Driver for %s responds with a strange error code: %ld\n", ctx.name, err); 
            break;
    }

    if(drv->lpVtbl->init(drv, NULL)) {
        char drivername[124];
        drv->lpVtbl->getDriverName(drv, drivername);
        printf("Driver initialization succeeded. Reported name: %s\n", drivername);
    } else {
        char errorMessage[124];
        drv->lpVtbl->getErrorMessage(drv, errorMessage);
        printf("Driver initialization failed. Error: %s\n", errorMessage);
    }

cleanup:
    if(drv)
        drv->lpVtbl->release(drv);
    printf("Driver released.\n");
    free(ctx.id);
    free(ctx.descr);
    return 0;
}

/** @}*/
