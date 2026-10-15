#include "./state.h"
#include "./input.h"
#include "./parse.h"
#include "./cmd.h"
#include "../../base/rand.h"
#include "../layout/disk/raw_img.h"

static void cmdSetAutoYes(const ConstSharedInfo* pInfo, ProgState* pState);
static void cmdSetAlwaysBinary(const ConstSharedInfo* pInfo, ProgState* pState);

static es_t cmdCls(const ConstSharedInfo* pInfo);
static es_t cmdHelp(const ConstSharedInfo* pInfo);
static es_t cmdVersion(const ConstSharedInfo* pInfo);

// Will adopt path from command info
static es_t cmdCreateDisk(const ConstSharedInfo* pInfo, CmdInfo* pCmdInfo, layout_DiskInfo* pDiskInfo);

static es_t cmdClose(const ConstSharedInfo* pInfo, layout_DiskInfo* pDiskInfo);
// Returns true for confirmed exit
static bool cmdExit(const ConstSharedInfo* pInfo);

static void cmdEG(const ConstSharedInfo* pInfo);


// Parsing code may not R/W program state directly
//   so the program logic remains simple 🤯
static es_t parseBeginning(const View8* pCmdView, const Dview* pSegments,
                            SrcType srcType, bool alwaysBinaryUnits,
                            Cmd* pResultCmd, size_t* pAfterBeginningI);

es_t core(const Dview* pArgs, ProgState* pState)
{
    String8 cmdString = {0};
    Dview cmdSegments = {0};
    es_t es = 0;
    // Shared info must be outside loop for freeing memory and closing disk after the loop ends
    ConstSharedInfo sharedInfo = {
        .srcType = pState->srcState.type,

        .autoYes = pState->autoYes,
        .alwaysBinaryUnits = pState->alwaysBinaryUnits,

        .pDiskInfo = NULL,
        
        .pCmdView = NULL,
        .pCmdSegments = NULL,
        .pCmd = NULL,
        .afterBeginningI = 0
    };
    while (true)
    {
        // 16 errors max from file or direct args
        if (pState->srcState.type != SRCTYPE_TERMINAL_LINES && pState->errorCount > 16)
            fretvoid;
        // Get command text and text segments
        if (!getCmdText(&pState->srcState, &cmdString, &cmdSegments))
            fretvoid;
        View8 cmdView = vwstr(&cmdString);
        // Parse command beginning
        Cmd cmd = {0};
        size_t afterBeginningI = 0;
        es = parseBeginning(&cmdView, &cmdSegments,
                                pState->srcState.type, pState->alwaysBinaryUnits,
                                &cmd, &afterBeginningI);
        // Don't add on error
        if (es != ES_SUCCESS)
        {
            pState->errorCount++;
            continue;
        }
        // Ignore null
        else if (cmd.type == CMDTYPE_NULL)
        {
            continue;
        }
        // Setup shared info for command functions
        sharedInfo = (ConstSharedInfo){
            .srcType = pState->srcState.type,

            .autoYes = pState->autoYes,
            .alwaysBinaryUnits = pState->alwaysBinaryUnits,

            .pDiskInfo = &pState->diskInfo,
            
            .pCmdView = &cmdView,
            .pCmdSegments = &cmdSegments,
            .pCmd = &cmd,
            .afterBeginningI = afterBeginningI
        };
        // Run the command 🙀
        switch (cmd.type)
        {
        case CMDTYPE_SET_AUTO_YES:
            cmdSetAutoYes(&sharedInfo, pState);
            break;
        case CMDTYPE_SET_ALWAYS_BINARY:
            cmdSetAlwaysBinary(&sharedInfo, pState);
            break;
        case CMDTYPE_CLS:
            es = cmdCls(&sharedInfo);
            if (!es) pState->errorCount++;
            break;
        case CMDTYPE_HELP:
            es = cmdHelp(&sharedInfo);
            if (!es) pState->errorCount++;
            break;
        case CMDTYPE_VERSION:
            es = cmdVersion(&sharedInfo);
            if (!es) pState->errorCount++;
            break;
        case CMDTYPE_CREATE_DISK:
            es = cmdCreateDisk(&sharedInfo, &cmd.info, &pState->diskInfo);
            if (!es) pState->errorCount++;
            break;
        // case CMDTYPE_SELECT_DISK:
        //     cmdSelectDisk(pState, &cmd.info);
        //     break;
        // case CMDTYPE_SAVE:
        //     cmdSave(pState, &vwstr(&cmdString));
        //     break;
        case CMDTYPE_CLOSE:
            es = cmdClose(&sharedInfo, &pState->diskInfo);
            if (!es) pState->errorCount++;
            break;
        case CMDTYPE_EXIT:
            if (cmdExit(&sharedInfo))
                fretvoid;
            break;
        case CMDTYPE_EG_444:
        case CMDTYPE_EG_DIFF_VHD:
        case CMDTYPE_EG_BIG_K:
        case CMDTYPE_EG_BIG_J:
        case CMDTYPE_EG_BIG_P:
        case CMDTYPE_EG_UEFI_SPEC:
        case CMDTYPE_EG_FRANCAIS:
        case CMDTYPE_EG_IGHRIQI:
        case CMDTYPE_EG_SERBIAN:
        case CMDTYPE_EG_DUTCHLANDIAN:
            cmdEG(&sharedInfo);
            break;
        default:
        {
            base_fatalErrorf("Unprogrammed command type %u given to %s.\n",
                            cmd.type, __func__);
            base_halt(ES_FAILURE);
        } break;
        }
    }
end:
    // Close opened disk and free layout-specfic metadata memory
    switch (pState->diskInfo.diskLayout.type)
    {
    case LAYOUT_TYPE_NULL:
    case LAYOUT_TYPE_UNKNOWN:
        break;
    case LAYOUT_TYPE_DISK_RAW_IMAGE:
        lrawimgExit(&sharedInfo);
        break;
    default:
        base_fatalErrorf("Unprogrammed disk type = %u at end of %s.\n",
                        pState->diskInfo.diskLayout.type, __func__);
        base_halt(ES_FAILURE);
    }
    string8Free(&cmdString);
    dviewFree(&cmdSegments);
    return (pState->errorCount == 0)? ES_SUCCESS : ES_FAILURE;
}



static es_t parseBeginning(const View8* pCmdView, const Dview* pSegments,
                            SrcType srcType, bool alwaysBinaryUnits,
                            Cmd* pResultCmd, size_t* pAfterBeginningI)
{
    // For quickly passing these 2 args to called functions
    ParseSharedInfo sharedInfo = (ParseSharedInfo){
        .pCmdView = pCmdView,
        .srcType = srcType,
        .alwaysBinaryUnits = alwaysBinaryUnits
    };
    // Argument declarator infos for parsing (freed at end of function)
    Dadinfo declInfos = {0};

    Cmd resultCmd = {0}; // Type kept as CMDTYPE_NULL on errors
    bool error = false; // Set to true on error discovery
    bool internalSkip = false; // If true: CMDTYPE_NULL is a skipped command rather than an erroneous one

    // Initialize after-beginning index to SIZE_MAX for debugging
    *pAfterBeginningI = SIZE_MAX;

    // All whitespace no segments
    if (dviewEmpty(pSegments))
    {
        internalSkip = true;
    }
    
    // Set yes
    else if (utils_compareLowNT(&at(pSegments, 0), "autoyes"))
    {
        dadinfoAppendV(&declInfos, adinfoMake("autoyes", true, false, NULL, 0));

        if (parseCmd(pSegments, 0, &declInfos, &sharedInfo, pAfterBeginningI)
            != ES_SUCCESS) error = true;
        else
            resultCmd = cmdMake(CMDTYPE_SET_AUTO_YES, (CmdInfo){.switchValue = true});
    }
    else if (utils_compareLowNT(&at(pSegments, 0), "manyes"))
    {
        dadinfoAppendV(&declInfos, adinfoMake("manyes", true, false, NULL, 0));

        if (parseCmd(pSegments, 0, &declInfos, &sharedInfo, pAfterBeginningI)
            != ES_SUCCESS) error = true;
        else
            resultCmd = cmdMake(CMDTYPE_SET_AUTO_YES, (CmdInfo){.switchValue = false});
    }
    
    // Set binary
    else if (utils_compareLowNT(&at(pSegments, 0), "binary"))
    {
        dadinfoAppendV(&declInfos, adinfoMake("binary", true, false, NULL, 0));

        if (parseCmd(pSegments, 0, &declInfos, &sharedInfo, pAfterBeginningI)
            != ES_SUCCESS) error = true;
        else
            resultCmd = cmdMake(CMDTYPE_SET_ALWAYS_BINARY, (CmdInfo){.switchValue = true});
    }
    else if (utils_compareLowNT(&at(pSegments, 0), "decimal"))
    {
        dadinfoAppendV(&declInfos, adinfoMake("decimal", true, false, NULL, 0));

        if (parseCmd(pSegments, 0, &declInfos, &sharedInfo, pAfterBeginningI)
            != ES_SUCCESS) error = true;
        else
            resultCmd = cmdMake(CMDTYPE_SET_ALWAYS_BINARY, (CmdInfo){.switchValue = false});
    }
    
    // Clear screen
    else if (utils_compareLowNT(&at(pSegments, 0), "cls"))
    {
        dadinfoAppendV(&declInfos, adinfoMake("cls", true, false, NULL, 0));

        if (parseCmd(pSegments, 0, &declInfos, &sharedInfo, pAfterBeginningI)
            != ES_SUCCESS) error = true;
        else
            resultCmd = cmdMake(CMDTYPE_CLS, (CmdInfo){0});
    }

    // help command
    else if (utils_compareLowNT(&at(pSegments, 0), "help"))
    {
        dadinfoAppendV(&declInfos, adinfoMake("help", true, false, NULL, 0));

        if (parseCmd(pSegments, 0, &declInfos, &sharedInfo, pAfterBeginningI)
            != ES_SUCCESS) error = true;
        else
            resultCmd = cmdMake(CMDTYPE_HELP, (CmdInfo){0});
    }

    // Version command
    else if (utils_compareLowNT(&at(pSegments, 0), "version"))
    {
        dadinfoAppendV(&declInfos, adinfoMake("version", true, false, NULL, 0));

        if (parseCmd(pSegments, 0, &declInfos, &sharedInfo, pAfterBeginningI)
            != ES_SUCCESS) error = true;
        else
            resultCmd = cmdMake(CMDTYPE_VERSION, (CmdInfo){0});
    }

    // Create disk
    else if (utils_compareLowNT(&at(pSegments, 0), "create"))
    {
        // e.g. select x.img sectsize 4KiB
        ExtractedAV path = {0};

        // If only one segment is in the array it will print an error
        dadinfoAppendV(&declInfos, adinfoMake("create", true, true, (AVInfo[]){
            avinfoMakeTextual("disk image path", TEXTAVTYPE_CREATABLE_DISK_PATH, NULL, 0, NULL, &path)
        }, 1));

        if (parseCmd(pSegments, 0, &declInfos, &sharedInfo, pAfterBeginningI)
            != ES_SUCCESS) error = true;
        else
        {
            // Set disk type first 
            //   (if it's late then if it fails
            //   code would have to reset the command object)
            // NOTE: TODO:
            //   THIS DOESN'T COVER VOLUME PATHS LIKE "C:/"
            //   A CALL TO A FUNCTION TO GET THE PHYSICAL DISK PATH FROM THE VOLUME
            //   AND TO ASK USER FOR CONFIRMATION
            //   MUST BE ADDED LATER
            utils_PathType pt = utils_getPathType(&vwstr(&path.textualVal));
            if (pt == UTILS_PATHTYPE_DISK)
            {
                resultCmd.info.createDisk.diskType = LAYOUT_TYPE_DISK_REAL_DISK;
            }
            else
            {
                utils_FileType ft = utils_getFileType(&vwstr(&path.textualVal));
                switch (ft)
                {
                case UTILS_FILETYPE_RAW:
                    resultCmd.info.createDisk.diskType = LAYOUT_TYPE_DISK_RAW_IMAGE;
                    break;
                default:
                    base_stdErrorf("Unsupported disk image file type \"%s\".\n",
                                    string8NT(&path.textualVal));
                    string8Free(&path.textualVal);
                    error = true;
                    break;
                }
            }
            if (!error)
            {
                // Set command type
                resultCmd.type = CMDTYPE_CREATE_DISK;
                // Adopt path
                cmdInfoCreateDiskAdoptPath(&resultCmd.info, &path.textualVal);
            }
        }
    }

    // Close
    else if (utils_compareLowNT(&at(pSegments, 0), "close"))
    {
        dadinfoAppendV(&declInfos, adinfoMake("close", true, false, NULL, 0));

        if (parseCmd(pSegments, 0, &declInfos, &sharedInfo, pAfterBeginningI)
            != ES_SUCCESS) error = true;
        else
            resultCmd = cmdMake(CMDTYPE_CLOSE, (CmdInfo){0});
    }

    // Exit
    else if (utils_compareLowNT(&at(pSegments, 0), "exit"))
    {
        dadinfoAppendV(&declInfos, adinfoMake("exit", true, false, NULL, 0));

        if (parseCmd(pSegments, 0, &declInfos, &sharedInfo, pAfterBeginningI)
            != ES_SUCCESS) error = true;
        else
            resultCmd = cmdMake(CMDTYPE_EXIT, (CmdInfo){0});
    }

    // Easter egg: The UEFI spec defines a 2-byte value at MBR offset 444 as "Unknown", which is funny
    else if (utils_compareLowNT(&at(pSegments, 0), "444")
    && dviewSize(pSegments) == 1)
    {
        resultCmd = cmdMake(CMDTYPE_EG_444, (CmdInfo){0});
    }
    // Easter egg: Differencing VHD is an evil format
    else if (utils_compareLowNT(&at(pSegments, 0), "differencing")
    && dviewSize(pSegments) == 2
    && utils_compareLowNT(&at(pSegments, 1), "vhd"))
    {
        resultCmd = cmdMake(CMDTYPE_EG_DIFF_VHD, (CmdInfo){0});
    }
    // Easter egg: Big K has various funny quotes
    else if (utils_compareLowNT(&at(pSegments, 0), "big")
    && dviewSize(pSegments) == 2
    && utils_compareLowNT(&at(pSegments, 1), "k"))
    {
        resultCmd = cmdMake(CMDTYPE_EG_BIG_K, (CmdInfo){0});
    }
    // Easter egg: Big J says "mbr is shit"
    else if (utils_compareLowNT(&at(pSegments, 0), "big")
    && dviewSize(pSegments) == 2
    && utils_compareLowNT(&at(pSegments, 1), "j"))
    {
        resultCmd = cmdMake(CMDTYPE_EG_BIG_J, (CmdInfo){0});
    }
    // Easter egg: Big P says "What's an OS?"
    else if (utils_compareLowNT(&at(pSegments, 0), "big")
    && dviewSize(pSegments) == 2
    && utils_compareLowNT(&at(pSegments, 1), "p"))
    {
        resultCmd = cmdMake(CMDTYPE_EG_BIG_P, (CmdInfo){0});
    }
    // Easter egg: "Help me" in french
    else if (utils_compareLowNT(&at(pSegments, 0), "aide")
    && dviewSize(pSegments) == 2
    && utils_compareLowNT(&at(pSegments, 1), "moi"))
    {
        resultCmd = cmdMake(CMDTYPE_EG_FRANCAIS, (CmdInfo){0});
    }
    // Easter egg: "Help me" in greek
    else if (utils_compareLowNT(&at(pSegments, 0), u8"βοηθήστε")
    && dviewSize(pSegments) == 2
    && utils_compareLowNT(&at(pSegments, 1), u8"με"))
    {
        resultCmd = cmdMake(CMDTYPE_EG_IGHRIQI, (CmdInfo){0});
    }
    // Easter egg: "Help me" in serbian
    else if (utils_compareLowNT(&at(pSegments, 0), u8"помози")
    && dviewSize(pSegments) == 2
    && utils_compareLowNT(&at(pSegments, 1), u8"ми"))
    {
        resultCmd = cmdMake(CMDTYPE_EG_SERBIAN, (CmdInfo){0});
    }
    // Easter egg: "Help me" in german
    else if (utils_compareLowNT(&at(pSegments, 0), "hilf")
    && dviewSize(pSegments) == 2
    && utils_compareLowNT(&at(pSegments, 1), "mir"))
    {
        resultCmd = cmdMake(CMDTYPE_EG_DUTCHLANDIAN, (CmdInfo){0});
    }
    // Easter egg: We obey the UEFI spec
    else if (utils_compareLowNT(&at(pSegments, 0), "uefi")
    && dviewSize(pSegments) == 2
    && utils_compareLowNT(&at(pSegments, 1), "spec"))
    {
        resultCmd = cmdMake(CMDTYPE_EG_UEFI_SPEC, (CmdInfo){0});
    }

    // Unsupported command
    if (!internalSkip && !error && resultCmd.type == CMDTYPE_NULL)
    {
        printInvalidCmdErrorSh(&vw("unsupported command"), &vw(""), &sharedInfo);
        error = true;
    }

    // Return
    dadinfoFree(&declInfos);
    *pResultCmd = resultCmd;
    return (!error)? ES_SUCCESS : ES_FAILURE;
}



// Returns ES_FAILURE if no open disk
static inline es_t ensureOpenDisk(const ConstSharedInfo* pInfo)
{
    if (pInfo->pDiskInfo->handle == HANDLE_NONE)
    {
        printInvalidCmdError(&vw("no selected disk"), &vw(""),
                            pInfo->pCmdView, pInfo->srcType);
        return ES_FAILURE;
    }
    return ES_SUCCESS;
}



static void cmdSetAutoYes(const ConstSharedInfo* pInfo, ProgState* pState)
{
    pState->autoYes = pInfo->pCmd->info.switchValue;
}

static void cmdSetAlwaysBinary(const ConstSharedInfo* pInfo, ProgState* pState)
{
    pState->alwaysBinaryUnits = pInfo->pCmd->info.switchValue;
}



static es_t cmdCls(const ConstSharedInfo* pInfo)
{
    if (pInfo->srcType != SRCTYPE_TERMINAL_LINES)
    {
        printInvalidCmdError(&vw("Command may only be used in terminal lines input"), &vw(""),
                                pInfo->pCmdView, pInfo->srcType);
        return ES_FAILURE;
    }
    if (!pInfo->autoYes
    && pInfo->pDiskInfo->edited)
    {
        base_printf("Are you sure you want to clear the screen?\n");
        if (!utils_confirmation()) return ES_SUCCESS;
    }
    base_printf(COLOUR_CLS);
    return ES_SUCCESS;
}

static es_t cmdHelp(const ConstSharedInfo* pInfo)
{
    if (pInfo->srcType != SRCTYPE_TERMINAL_LINES)
    {
        printInvalidCmdError(&vw("Command may only be used in terminal lines input"), &vw(""),
                                pInfo->pCmdView, pInfo->srcType);
        return ES_FAILURE;
    }
    utils_help();
    return ES_SUCCESS;
}

static es_t cmdVersion(const ConstSharedInfo* pInfo)
{
    if (pInfo->srcType != SRCTYPE_TERMINAL_LINES)
    {
        printInvalidCmdError(&vw("Command may only be used in terminal lines input"), &vw(""),
                                pInfo->pCmdView, pInfo->srcType);
        return ES_FAILURE;
    }
    utils_version();
    return ES_SUCCESS;
}



static es_t cmdCreateDisk(const ConstSharedInfo* pInfo, CmdInfo* pCmdInfo, layout_DiskInfo* pDiskInfo)
{
    if (!pInfo->autoYes && pInfo->pDiskInfo->handle != HANDLE_NONE)
    {
        base_printf("Are you sure you want to select a new disk image?\n");
        if (!utils_confirmation()) return ES_SUCCESS;
    }

    switch (pInfo->pCmd->info.createDisk.diskType)
    {
    case LAYOUT_TYPE_DISK_RAW_IMAGE:
        return lrawimgCmdCreate(pInfo, pCmdInfo, pDiskInfo);
    default:
        base_fatalErrorf("Unprogrammed disk type = %u given to %s.\n",
                        pInfo->pCmd->info.createDisk.diskType, __func__);
        base_halt(ES_FAILURE);
    }
}



static es_t cmdClose(const ConstSharedInfo* pInfo, layout_DiskInfo* pDiskInfo)
{
    if (!ensureOpenDisk(pInfo))
        return ES_FAILURE;
    if (!pInfo->autoYes && pInfo->pDiskInfo->edited)
    {
        base_printf("Are you sure you want to close the disk?\n");
        if (!utils_confirmation()) return ES_SUCCESS;
    }

    switch (pInfo->pDiskInfo->diskLayout.type)
    {
    case LAYOUT_TYPE_DISK_RAW_IMAGE:
        return lrawimgCmdClose(pInfo, pDiskInfo);
    default:
        base_fatalErrorf("Unprogrammed disk type = %u given to %s.\n",
                        pInfo->pDiskInfo->diskLayout.type, __func__);
        base_halt(ES_FAILURE);
    }
}

static bool cmdExit(const ConstSharedInfo* pInfo)
{
    if (!pInfo->autoYes && pInfo->pDiskInfo->edited)
    {
        base_printf("Are you sure you want to exit the program?\n");
        return utils_confirmation();
    }
    return true;
}



static void cmdEG(const ConstSharedInfo* pInfo)
{
    switch (pInfo->pCmd->type)
    {
    case CMDTYPE_EG_444:
        base_printf("We don't talk about it son...\n");
        break;
    case CMDTYPE_EG_DIFF_VHD:
        base_printf("Very evil\n");
        break;
    case CMDTYPE_EG_BIG_K:
    {
        static const UTF8_t* K_QUOTES[] = {
            "well lets not talk abt it",
            "like syscalls and nvm shit",
            "Or else.",
            "Well I won't recommend running it on real hardware"
        };
        base_printf("%s\n", K_QUOTES[base_badRand() % 4]);
    } break;
    case CMDTYPE_EG_BIG_J:
        base_printf("mbr is shit\n");
        break;
    case CMDTYPE_EG_BIG_P:
        base_printf("What's an OS?\n");
        break;
    case CMDTYPE_EG_UEFI_SPEC:
        base_printf(u8"The UEFI spec we listen and obey 🫡"
                    " https://uefi.org/sites/default/files/resources/UEFI_Spec_Final_2.11.pdf" "\n");
        break;
    case CMDTYPE_EG_FRANCAIS:
        base_printf(u8"je parle pas français 😔\n");
        break;
    case CMDTYPE_EG_IGHRIQI:
        base_printf(u8"Δεν μιλάω Ελληνικά 😔\n");
        break;
    case CMDTYPE_EG_SERBIAN:
        base_printf(u8"Не причам српски 😔\n");
        break;
    case CMDTYPE_EG_DUTCHLANDIAN:
        base_printf("Ich spreche nicht Deutsch 😔\n");
        break;
    default:
        base_fatalErrorf("Unprogrammed easter egg command type %u given to %s.\n",
                        pInfo->pCmd->type, __func__);
        base_halt(ES_FAILURE);
    }
}
