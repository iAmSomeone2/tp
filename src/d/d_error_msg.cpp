/**
 * d_error_msg.cpp
 * Shutdown / return-to-menu fade handler
 */

#include "d/dolzel.h" // IWYU pragma: keep

#include "d/d_error_msg.h"
#include "m_Do/m_Do_Reset.h"
#include "m_Do/m_Do_graphic.h"

static u8 l_captureAlpha = 0xFF;

static void drawCapture(u8 alpha) {
    static bool l_texCopied = false;

    if (!l_texCopied) {
        GXSetTexCopySrc(0, 0, FB_WIDTH, FB_HEIGHT);
        GXSetTexCopyDst(FB_WIDTH / 2, FB_HEIGHT / 2, (GXTexFmt)mDoGph_gInf_c::getFrameBufferTimg()->format, GX_TRUE);
        GXCopyTex(mDoGph_gInf_c::getFrameBufferTex(), GX_FALSE);
        l_texCopied = true;
    }

    mDoGph_gInf_c::setClearColor(g_clearColor);
    mDoGph_gInf_c::beginRender();
    GXSetAlphaUpdate(GX_FALSE);
    j3dSys.drawInit();

    GXInitTexObj(mDoGph_gInf_c::getFrameBufferTexObj(), mDoGph_gInf_c::getFrameBufferTex(), FB_WIDTH / 2, FB_HEIGHT / 2, (GXTexFmt)mDoGph_gInf_c::getFrameBufferTimg()->format, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXInitTexObjLOD(mDoGph_gInf_c::getFrameBufferTexObj(), GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
    GXLoadTexObj(mDoGph_gInf_c::getFrameBufferTexObj(), GX_TEXMAP0);
    GXSetNumChans(0);
    GXSetNumIndStages(0);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 60, GX_FALSE, GX_PTIDENTITY);
    GXSetNumTevStages(1);

    GXColor color = {0, 0, 0, alpha};
    GXSetTevColor(GX_TEVREG0, color);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_A0, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetZCompLoc(GX_TRUE);
    GXSetZMode(GX_DISABLE, GX_ALWAYS, GX_DISABLE);
    GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
    GXSetFog(GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, g_clearColor);
    GXSetFogRangeAdj(GX_DISABLE, 0, NULL);
    GXSetCullMode(GX_CULL_NONE);
    GXSetDither(GX_ENABLE);

    Mtx44 m;
    C_MTXOrtho(m, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 10.0f);
    GXLoadPosMtxImm(g_mDoMtx_identity, GX_PNMTX0);
    GXSetProjection(m, GX_ORTHOGRAPHIC);
    GXSetCurrentMtx(0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_CLR_RGB, GX_RGB8, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_CLR_RGBA, GX_RGB8, 0);

    mDoGph_drawFilterQuad(1, 1);
    mDoGph_gInf_c::endRender();
    JFWDisplay::getManager()->resetFader();
}

bool dShutdownErrorMsg_c::execute() {
    if (!mDoRst::isShutdown() && !mDoRst::isReturnToMenu()) {
        return false;
    }

    if (l_captureAlpha == 0xFF) {
        if (Z2AudioMgr::getInterface()->isResetting() && !mDoAud_resetRecover()) {
            drawCapture(l_captureAlpha);
            return true;
        }

        if (mDoAud_zelAudio_c::isInitFlag()) {
            Z2AudioMgr::getInterface()->resetProcess(0x10, true);
        }
    }

    drawCapture(l_captureAlpha);

    if (cLib_chaseUC(&l_captureAlpha, 0, 15) != 0) {
        if (mDoRst::isReturnToMenu()) {
            mDoRst_reset(1, 0x80000000, 0);
        } else {
            mDoRst_reset(1, 1, 1);
        }
    }

    return true;
}
