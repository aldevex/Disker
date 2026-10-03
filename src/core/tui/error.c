#include "./tui.h"

void printInvalidCmdError(const View8* pReasonView, const View8* pSpecifiedTextView_optional,
                            const View8* pCmdView, SrcType srcType)
{
    if (srcType != SRCTYPE_TERMINAL_LINES)
    {
        if (pSpecifiedTextView_optional == NULL || view8Empty(pSpecifiedTextView_optional))
        {
            fprintf(stderr, "invalid command \"%.*s\" (%.*s)\n",
                    pfSpread(pCmdView), pfSpread(pReasonView));
        }
        else
        {
            fprintf(stderr, "invalid command \"%.*s\" (%.*s \"%.*s\")\n",
                    pfSpread(pCmdView), pfSpread(pReasonView),
                    pfSpread(pSpecifiedTextView_optional));
        }
    }
    else
    {
        if (pSpecifiedTextView_optional == NULL || view8Empty(pSpecifiedTextView_optional))
        {
            fprintf(stderr, "invalid command (%.*s)\n", 
                    pfSpread(pReasonView));
        }
        else
        {
            fprintf(stderr, "invalid command (%.*s \"%.*s\")\n", 
                    pfSpread(pReasonView), pfSpread(pSpecifiedTextView_optional));
        }
    }
}
