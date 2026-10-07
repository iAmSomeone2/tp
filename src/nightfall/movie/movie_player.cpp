//
// Created by brenden on 10/7/26.
//

#include <d/actor/d_a_movie_player.h>
#include <dolphin/types.h>
#include <f_op/f_op_actor.h>
#include <f_op/f_op_actor_mng.h>

#include "d/d_com_inf_game.h"
#include "f_op/f_op_overlap_mng.h"
#include "m_Do/m_Do_graphic.h"

#include <cstring>

namespace nf {
// The player is a singleton. `state`: 0 stopped/opened, 1 prepared, 2 playing, 3 finished, 4
// paused, 5 error.
static daMP_THPPlayer daMP_ActivePlayer;

static bool daMP_Fail_alloc = false;

/**
 * Starts (or resumes) playback from the prepared or paused state. Returns TRUE if it did.
 */
static int playMovie() {
    if (daMP_ActivePlayer.open != 0 &&
        (daMP_ActivePlayer.state == 1 || daMP_ActivePlayer.state == 4))
    {
        daMP_ActivePlayer.state = 2;
        daMP_ActivePlayer.prevCount = 0;
        daMP_ActivePlayer.curCount = 0;
        daMP_ActivePlayer.retaceCount = -1;
        return true;
    }

    return false;
}

/**
 * Pauses a playing movie. Returns 1 if it was playing.
 */
static int pauseMovie() {
    if (daMP_ActivePlayer.open != 0 && daMP_ActivePlayer.state == 2) {
        daMP_ActivePlayer.internalState = 4;
        daMP_ActivePlayer.state = 4;
        return 1;
    }

    return 0;
}

/**
 * Stops playback: restores the previous retrace callback, cancels the reader and decoder threads,
 * drops the pending textures and ends any volume ramp.
 */
static void stopMovie() {
    if (daMP_ActivePlayer.open != 0 && daMP_ActivePlayer.state != 0) {
        daMP_ActivePlayer.internalState = 0;
        daMP_ActivePlayer.state = 0;

        if (daMP_ActivePlayer.audioExist != 0) {
            // Kill any audio decode/play threads
        }

        // Remove any pending decoded textures from the queue
        // while (daMP_PopUsedTextureSet() != NULL) {}

        daMP_ActivePlayer.curVolume = daMP_ActivePlayer.targetVolume;
        daMP_ActivePlayer.rampCount = 0.0f;
    }
}

/**
 * Copies the video parameters into `info`. Returns 1 if a movie is open, else 0.
 */
static int getVideoInfo(THPVideoInfo* info) {
    if (daMP_ActivePlayer.open != 0) {
        memcpy(info, &daMP_ActivePlayer.videoInfo, sizeof(THPVideoInfo));
        return 1;
    }

    return 0;
}

/**
 * Copies the audio parameters into `info`. Returns 1 if a movie is open, else 0.
 */
static int getAudioInfo(THPAudioInfo* info) {
    if (daMP_ActivePlayer.open != 0) {
        memcpy(info, &daMP_ActivePlayer.audioInfo, sizeof(THPAudioInfo));
        return 1;
    }

    return 0;
}

/**
 * Number of frames in the movie, or 0 if none is open.
 */
static u32 getTotalMovieFrames() {
    if (daMP_ActivePlayer.open != 0) {
        return daMP_ActivePlayer.header.numFrames;
    }

    return 0;
}

static u32 getRemainingFrames() {
    uint32_t temp_r31;
    if (daMP_Fail_alloc != 0 || daMP_ActivePlayer.state == 5) {
        return 0;
    }

    if (daMP_ActivePlayer.open && daMP_ActivePlayer.dispTextureSet != nullptr) {
        temp_r31 = (daMP_ActivePlayer.dispTextureSet->frameNumber + daMP_ActivePlayer.initReadFrame) % daMP_ActivePlayer.header.numFrames;
    } else {
        return -1;
    }

    const uint32_t total_frames = getTotalMovieFrames();
    if (total_frames == 0) {
        return 0;
    }

    if (total_frames <= 1) {
        return 0;
    }

    if (total_frames - 1 <= temp_r31) {
        return 0;
    }

    return (total_frames - 1) - temp_r31;
}

static void setMovieVolume(f32) {}

/**
 * Current player state (0 stopped ... 5 error).
 */
static int getPlayerState() {
    return daMP_ActivePlayer.state;
}

static void drawFrame() {
    if (!fopOvlpM_IsPeek() && (cAPICPad_ANY_BUTTON(0) || !daMP_c::daMP_c_Get_MovieRestFrame())) {
        dComIfGp_event_reset();
        daMP_c::daMP_c_Set_PercentMovieVolume(0.0f);
    }
}
}

/**
 * Demo number from the actor parameters (bits 7-13).
 */
int daMP_c::daMP_c_Get_arg_demoNo() {
    return (fopAcM_GetParam(this) >> 7) & 0x7F;
}

/**
 * Movie number from the actor parameters (bits 0-6).
 */
int daMP_c::daMP_c_Get_arg_movieNo() {
    return fopAcM_GetParam(this) & 0x7F;
}

/**
 * Actor create: builds the path `/Movie/demo_movie<demoNo>_<movieNo>.thp`, starts the player
 * (remembering a failure in `daMP_Fail_alloc`, after which the actor does nothing) and exposes the
 * player controls through the function pointers used by `m_myObj` callers.
 */
int daMP_c::daMP_c_Init() {
    if (m_myObj != nullptr) {
        return cPhs_ERROR_e;
    }

    mDoGph_gInf_c::setFrameRate(1);

    // A movie that cannot be played is skipped, not refused: d_demo waits for m_myObj before it
    // continues, so failing creation would stall the cutscene.
    const int demoNo = daMP_c_Get_arg_demoNo();
    const int movieNo = daMP_c_Get_arg_movieNo();
    nf::daMP_Fail_alloc = demoNo > 99 || movieNo > 99;

    mpGetMovieRestFrame = nf::getRemainingFrames;
    mpSetPercentMovieVol = nf::setMovieVolume;
    mpTHPGetTotalFrame = nf::getTotalMovieFrames;
    mpTHPPlay = nf::playMovie;
    mpTHPPause = nf::pauseMovie;
    mpTHPStop = nf::stopMovie;

    m_myObj = this;
    return cPhs_COMPLETE_e;
}

int daMP_c::daMP_c_Finish() {
    nf::stopMovie();
    m_myObj = nullptr;
    return 1;
}

int daMP_c::daMP_c_Main() {
    // ponytail: no decoder yet, so every movie ends at once, as if skipped; the real player ends
    // the event from its draw path instead (button press or last frame).
    if (!fopOvlpM_IsPeek()) { mDoGph_gInf_c::fadeIn(1.0f); dComIfGp_event_reset(); }
    return 1;
}

int daMP_c::daMP_c_Draw() {
    return 1;
}

int daMP_c::daMP_c_Callback_Init(fopAc_ac_c* movie_player_ptr) {
    fopAcM_ct(movie_player_ptr, daMP_c);
    return reinterpret_cast<daMP_c*>(movie_player_ptr)->daMP_c_Init();
}

int daMP_c::daMP_c_Callback_Finish(daMP_c* movie_player_ptr) {
    return movie_player_ptr->daMP_c_Finish();
}

int daMP_c::daMP_c_Callback_Main(daMP_c* movie_player_ptr) {
    return movie_player_ptr->daMP_c_Main();
}

int daMP_c::daMP_c_Callback_Draw(daMP_c* movie_player_ptr) {
    return movie_player_ptr->daMP_c_Draw();
}

/**
 * Process "is delete" method: not needed, always returns 1.
 */
static int daMP_Callback_Dummy(daMP_c*) {
    return 1;
}

static actor_method_class daMP_METHODS = {
    reinterpret_cast<process_method_func>(daMP_c::daMP_c_Callback_Init),
    reinterpret_cast<process_method_func>(daMP_c::daMP_c_Callback_Finish),
    reinterpret_cast<process_method_func>(daMP_c::daMP_c_Callback_Main),
    reinterpret_cast<process_method_func>(daMP_Callback_Dummy),
    reinterpret_cast<process_method_func>(daMP_c::daMP_c_Callback_Draw),
};

/**
 * Process profile of the movie player actor (list ID 7).
 */
actor_process_profile_definition g_profile_MOVIE_PLAYER = {
    /* Layer ID     */ fpcLy_CURRENT_e,
    /* List ID      */ 7,
    /* List Prio    */ fpcPi_CURRENT_e,
    /* Proc Name    */ fpcNm_MOVIE_PLAYER_e,
    /* Proc SubMtd  */ &g_fpcLf_Method.base,
    /* Size         */ sizeof(daMP_c),
    /* Size Other   */ 0,
    /* Parameters   */ 0,
    /* Leaf SubMtd  */ &g_fopAc_Method.base,
    /* Draw Prio    */ fpcDwPi_MOVIE_PLAYER_e,
    /* Actor SubMtd */ &daMP_METHODS,
    /* Status       */ fopAcStts_UNK_0x40000_e | fopAcStts_NOPAUSE_e | fopAcStts_STAFF_PRIMARY_e |
        fopAcStts_UNK_0x4000_e,
    /* Group        */ fopAc_ACTOR_e,
    /* Cull Type    */ fopAc_CULLBOX_CUSTOM_e,
};