/* CITS2002 Project 2018
   Name(s):		Padraig Lamont
   Student number(s):	22494652
 */

#include "targets.h"


BakeError target_allocate(char * name, Target * out) {
    out->name = name;
    out->state = TARGET_NOT_EXECUTED;

    // Allocate a buffer to hold all the dependencies of the target
    BakeError err = buf_allocate(&out->dependencies, 4 * sizeof(char *));
    if(err != BAKE_SUCCESS)
        return err;

    // Allocate a buffer to hold all the action lines of the target
    err = buf_allocate(&out->actionLines, 8 * sizeof(ActionLine));
    if(err != BAKE_SUCCESS) {
        buf_free(&out->dependencies);
        return err;
    }

    return BAKE_SUCCESS;
}


void target_free(Target * target) {
    buf_free(&target->dependencies);
    buf_free(&target->actionLines);
}


char ** target_getDependencies(Target * target) {
    // Return the dependencies array, which contains an array of char *'s
    return buf_get(&target->dependencies);
}


size_t target_dependencyCount(Target * target) {
    // Return the number of char *'s that are in the used data of the dependencies buffer
    return target->dependencies.used / sizeof(char *);
}


BakeError target_addDependency(Target * target, char * dependency) {
    // Append the dependency pointer to the dependencies array
    return buf_append(&target->dependencies, &dependency, sizeof(char *));
}


ActionLine * target_getActionLines(Target * target) {
    // Return the actionLines array, which contains an array of ActionLine's
    return buf_get(&target->actionLines);
}


size_t target_actionLineCount(Target * target) {
    // Return the number of ActionLine's that are in the used data of the targets buffer
    return target->actionLines.used / sizeof(ActionLine);
}


BakeError target_addActionLine(Target * target, ActionLine actionLine) {
    // Append actionLine to the actionLines array
    return buf_append(&target->actionLines, &actionLine, sizeof(ActionLine));
}


BakeError bakefile_allocate(Bakefile * out) {
    out->firstTarget = NULL;

    // Allocate a new StringMap for storing targets
    return strmap_allocate(&out->targets, 4);
}


void bakefile_free(Bakefile * bakefile) {
    // firstTarget will be free'd by the call to free the targets StringMap
    bakefile->firstTarget = NULL;

    // Free the resources of each target
    size_t entryCount = strmap_size(&bakefile->targets);
    StringMapEntry * entries = strmap_entries(&bakefile->targets);

    for(size_t index = 0; index < entryCount; ++index) {
        StringMapEntry entry = entries[index];
        Target * target = entry.value;

        // Free target
        target_free(target);
    }

    // Free the targets map itself
    strmap_free(&bakefile->targets);
}


Target * bakefile_getTarget(Bakefile * bakefile, char * identifier) {
    // Get the target value from the targets StringMap
    return strmap_get(&bakefile->targets, identifier);
}


BakeError bakefile_addTarget(Bakefile * bakefile, char * identifier, Target * target) {
    // Make sure there isn't already a target with the same identifier
    Target * existing = bakefile_getTarget(bakefile, identifier);
    if(existing != NULL) {
        reportError("Cannot have multiple targets with the same name %s\n", identifier);
        return BAKE_ERROR_ARGUMENTS;
    }

    // If this is the first target added to bakefile, set its firstTarget property
    if(strmap_size(&bakefile->targets) == 0) {
        bakefile->firstTarget = target;
    }

    // Add this new target to the targets map
    return strmap_put(&bakefile->targets, identifier, target);
}
