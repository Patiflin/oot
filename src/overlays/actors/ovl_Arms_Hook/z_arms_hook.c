#include "z_arms_hook.h"
#include "objects/object_link_boy/object_link_boy.h"

#define FLAGS (ACTOR_FLAG_4 | ACTOR_FLAG_5)

void ArmsHook_Init(Actor* thisx, GlobalContext* globalCtx);
void ArmsHook_Destroy(Actor* thisx, GlobalContext* globalCtx);
void ArmsHook_Update(Actor* thisx, GlobalContext* globalCtx);
void ArmsHook_Draw(Actor* thisx, GlobalContext* globalCtx);

void ArmsHook_Wait(ArmsHook* this, GlobalContext* globalCtx);
void ArmsHook_Shoot(ArmsHook* this, GlobalContext* globalCtx);

const ActorInit Arms_Hook_InitVars = {
    ACTOR_ARMS_HOOK,
    ACTORCAT_ITEMACTION,
    FLAGS,
    OBJECT_LINK_BOY,
    sizeof(ArmsHook),
    (ActorFunc)ArmsHook_Init,
    (ActorFunc)ArmsHook_Destroy,
    (ActorFunc)ArmsHook_Update,
    (ActorFunc)ArmsHook_Draw,
};

static ColliderQuadInit sQuadInit = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_PLAYER,
        AC_NONE,
        OC1_NONE,
        OC2_TYPE_PLAYER,
        COLSHAPE_QUAD,
    },
    {
        ELEMTYPE_UNK2,
        { 0x00000080, 0x00, 0x01 },
        { 0xFFCFFFFF, 0x00, 0x00 },
        TOUCH_ON | TOUCH_NEAREST | TOUCH_SFX_NORMAL,
        BUMP_NONE,
        OCELEM_NONE,
    },
    { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } } },
};

static Vec3f sUnusedVec1 = { 0.0f, 0.5f, 0.0f };
static Vec3f sUnusedVec2 = { 0.0f, 0.5f, 0.0f };

static Color_RGB8 sUnusedColors[] = {
    { 255, 255, 100 },
    { 255, 255, 50 },
};

static Vec3f D_80865B70 = { 0.0f, 0.0f, 0.0f };
static Vec3f D_80865B7C = { 0.0f, 0.0f, 900.0f };
static Vec3f D_80865B88 = { 0.0f, 500.0f, -3000.0f };
static Vec3f D_80865B94 = { 0.0f, -500.0f, -3000.0f };
static Vec3f D_80865BA0 = { 0.0f, 500.0f, 1200.0f };
static Vec3f D_80865BAC = { 0.0f, -500.0f, 1200.0f };

void ArmsHook_SetupAction(ArmsHook* this, ArmsHookActionFunc actionFunc) {
    this->actionFunc = actionFunc;
}

void ArmsHook_Init(Actor* thisx, GlobalContext* globalCtx) {
    ArmsHook* this = (ArmsHook*)thisx;

    Collider_InitQuad(globalCtx, &this->collider);
    Collider_SetQuad(globalCtx, &this->collider, &this->actor, &sQuadInit);
    ArmsHook_SetupAction(this, ArmsHook_Wait);
    this->unk_1E8 = this->actor.world.pos;
}

void ArmsHook_Destroy(Actor* thisx, GlobalContext* globalCtx) {
    ArmsHook* this = (ArmsHook*)thisx;

    if (this->grabbed != NULL) {
        this->grabbed->flags &= ~ACTOR_FLAG_13;
    }
    Collider_DestroyQuad(globalCtx, &this->collider);
}

void ArmsHook_Wait(ArmsHook* this, GlobalContext* globalCtx) {
    if (this->actor.parent == NULL) {
        Player* player = GET_PLAYER(globalCtx);
        // get correct timer length for hookshot or longshot
        s32 length = (player->heldItemActionParam == PLAYER_AP_HOOKSHOT) ? 13 : 26;

        ArmsHook_SetupAction(this, ArmsHook_Shoot);
        func_8002D9A4(&this->actor, 20.0f);
        this->actor.parent = &GET_PLAYER(globalCtx)->actor;
        this->timer = length;
    }
}

void func_80865044(ArmsHook* this) {
    this->actor.child = this->actor.parent;
    this->actor.parent->parent = &this->actor;
}

s32 ArmsHook_AttachToPlayer(ArmsHook* this, Player* player) {
    player->actor.child = &this->actor;
    player->heldActor = &this->actor;
    if (this->actor.child != NULL) {
        player->actor.parent = NULL;
        this->actor.child = NULL;
        return true;
    }
    return false;
}

void ArmsHook_DetachHookFromActor(ArmsHook* this) {
    if (this->grabbed != NULL) {
        this->grabbed->flags &= ~ACTOR_FLAG_13;
        this->grabbed = NULL;
    }
}

s32 ArmsHook_CheckForCancel(ArmsHook* this) {
    Player* player = (Player*)this->actor.parent;

    if (Player_HoldsHookshot(player)) {
        if ((player->itemActionParam != player->heldItemActionParam) || (player->actor.flags & ACTOR_FLAG_8) ||
            ((player->stateFlags1 & (PLAYER_STATE1_7 | PLAYER_STATE1_26)))) {
            this->timer = 0;
            ArmsHook_DetachHookFromActor(this);
            Math_Vec3f_Copy(&this->actor.world.pos, &player->unk_3C8);
            return 1;
        }
    }
    return 0;
}

void ArmsHook_AttachHookToActor(ArmsHook* this, Actor* actor) {
    actor->flags |= ACTOR_FLAG_13;
    this->grabbed = actor;
    Math_Vec3f_Diff(&actor->world.pos, &this->actor.world.pos, &this->grabbedDistDiff);
}

void ArmsHook_Shoot(ArmsHook* this, GlobalContext* globalCtx) {
    Player* player = GET_PLAYER(globalCtx);
    Actor* touchedActor;
    Actor* grabbed;
    Vec3f bodyDistDiffVec;
    Vec3f newPos;
    f32 bodyDistDiff;
    f32 phi_f16;
    DynaPolyActor* dynaPolyActor;
    f32 curGrabbedDist;
    f32 grabbedDist;
    s32 pad;
    CollisionPoly* poly;
    s32 bgId;
    Vec3f intersectPos;
    Vec3f prevFrameDiff;
    Vec3f sp60;
    f32 polyNormalX;
    f32 polyNormalZ;
    f32 velocity;
    s32 pad1;

    if ((this->actor.parent == NULL) || (!Player_HoldsHookshot(player))) {
        ArmsHook_DetachHookFromActor(this);
        Actor_Kill(&this->actor);
        return;
    }

    func_8002F8F0(&player->actor, NA_SE_IT_HOOKSHOT_CHAIN - SFX_FLAG);
    ArmsHook_CheckForCancel(this);

    if ((this->timer != 0) && (this->collider.base.atFlags & AT_HIT) &&
        (this->collider.info.atHitInfo->elemType != ELEMTYPE_UNK4)) {
        touchedActor = this->collider.base.at;
        if ((touchedActor->update != NULL) && (touchedActor->flags & (ACTOR_FLAG_9 | ACTOR_FLAG_10))) {
            if (this->collider.info.atHitInfo->bumperFlags & BUMP_HOOKABLE) {
                ArmsHook_AttachHookToActor(this, touchedActor);
                if (CHECK_FLAG_ALL(touchedActor->flags, ACTOR_FLAG_10)) {
                    func_80865044(this);
                }
            }
        }
        this->timer = 0;
        Audio_PlaySoundGeneral(NA_SE_IT_ARROW_STICK_CRE, &this->actor.projectedPos, 4, &D_801333E0, &D_801333E0,
                               &D_801333E8);
    } else if (DECR(this->timer) == 0) {
        grabbed = this->grabbed;
        if (grabbed != NULL) {
            if ((grabbed->update == NULL) || !CHECK_FLAG_ALL(grabbed->flags, ACTOR_FLAG_13)) {
                grabbed = NULL;
                this->grabbed = NULL;
            } else if (this->actor.child != NULL) {
                curGrabbedDist = Actor_WorldDistXYZToActor(&this->actor, grabbed);
                grabbedDist =
                    sqrtf(SQ(this->grabbedDistDiff.x) + SQ(this->grabbedDistDiff.y) + SQ(this->grabbedDistDiff.z));
                Math_Vec3f_Diff(&grabbed->world.pos, &this->grabbedDistDiff, &this->actor.world.pos);
                if ((curGrabbedDist - grabbedDist) > 50.0f) {
                    ArmsHook_DetachHookFromActor(this);
                    grabbed = NULL;
                }
            }
        }

        bodyDistDiff = Math_Vec3f_DistXYZAndStoreDiff(&player->unk_3C8, &this->actor.world.pos, &bodyDistDiffVec);
        if (bodyDistDiff < 30.0f) {
            velocity = 0.0f;
            phi_f16 = 0.0f;
        } else {
            if (this->actor.child != NULL) {
                velocity = 30.0f;
            } else if (grabbed != NULL) {
                velocity = 50.0f;
            } else {
                velocity = 200.0f;
            }
            phi_f16 = bodyDistDiff - velocity;
            if (bodyDistDiff <= velocity) {
                phi_f16 = 0.0f;
            }
            velocity = phi_f16 / bodyDistDiff;
        }

        newPos.x = bodyDistDiffVec.x * velocity;
        newPos.y = bodyDistDiffVec.y * velocity;
        newPos.z = bodyDistDiffVec.z * velocity;

        if (this->actor.child == NULL) {
            if ((grabbed != NULL) && (grabbed->id == ACTOR_BG_SPOT06_OBJECTS)) {
                Math_Vec3f_Diff(&grabbed->world.pos, &this->grabbedDistDiff, &this->actor.world.pos);
                phi_f16 = 1.0f;
            } else {
                Math_Vec3f_Sum(&player->unk_3C8, &newPos, &this->actor.world.pos);
                if (grabbed != NULL) {
                    Math_Vec3f_Sum(&this->actor.world.pos, &this->grabbedDistDiff, &grabbed->world.pos);
                }
            }
        } else {
            Math_Vec3f_Diff(&bodyDistDiffVec, &newPos, &player->actor.velocity);
            player->actor.world.rot.x =
                Math_Atan2S(sqrtf(SQ(bodyDistDiffVec.x) + SQ(bodyDistDiffVec.z)), -bodyDistDiffVec.y);
        }

        if (phi_f16 < 50.0f) {
            ArmsHook_DetachHookFromActor(this);
            if (phi_f16 == 0.0f) {
                ArmsHook_SetupAction(this, ArmsHook_Wait);
                if (ArmsHook_AttachToPlayer(this, player)) {
                    Math_Vec3f_Diff(&this->actor.world.pos, &player->actor.world.pos, &player->actor.velocity);
                    player->actor.velocity.y -= 20.0f;
                }
            }
        }
    } else {
        Actor_MoveForward(&this->actor);
        Math_Vec3f_Diff(&this->actor.world.pos, &this->actor.prevPos, &prevFrameDiff);
        Math_Vec3f_Sum(&this->unk_1E8, &prevFrameDiff, &this->unk_1E8);
        this->actor.shape.rot.x = Math_Atan2S(this->actor.speedXZ, -this->actor.velocity.y);
        sp60.x = this->unk_1F4.x - (this->unk_1E8.x - this->unk_1F4.x);
        sp60.y = this->unk_1F4.y - (this->unk_1E8.y - this->unk_1F4.y);
        sp60.z = this->unk_1F4.z - (this->unk_1E8.z - this->unk_1F4.z);
        if (BgCheck_EntityLineTest1(&globalCtx->colCtx, &sp60, &this->unk_1E8, &intersectPos, &poly, true, true, true,
                                    true, &bgId) &&
            !func_8002F9EC(globalCtx, &this->actor, poly, bgId, &intersectPos)) {
            polyNormalX = COLPOLY_GET_NORMAL(poly->normal.x);
            polyNormalZ = COLPOLY_GET_NORMAL(poly->normal.z);
            Math_Vec3f_Copy(&this->actor.world.pos, &intersectPos);
            this->actor.world.pos.x += 10.0f * polyNormalX;
            this->actor.world.pos.z += 10.0f * polyNormalZ;
            this->timer = 0;
            if (SurfaceType_IsHookshotSurface(&globalCtx->colCtx, poly, bgId)) {
                if (bgId != BGCHECK_SCENE) {
                    dynaPolyActor = DynaPoly_GetActor(&globalCtx->colCtx, bgId);
                    if (dynaPolyActor != NULL) {
                        ArmsHook_AttachHookToActor(this, &dynaPolyActor->actor);
                    }
                }
                func_80865044(this);
                Audio_PlaySoundGeneral(NA_SE_IT_HOOKSHOT_STICK_OBJ, &this->actor.projectedPos, 4, &D_801333E0,
                                       &D_801333E0, &D_801333E8);
            } else {
                CollisionCheck_SpawnShieldParticlesMetal(globalCtx, &this->actor.world.pos);
                Audio_PlaySoundGeneral(NA_SE_IT_HOOKSHOT_REFLECT, &this->actor.projectedPos, 4, &D_801333E0,
                                       &D_801333E0, &D_801333E8);
            }
        } else if (CHECK_BTN_ANY(globalCtx->state.input[0].press.button,
                                 (BTN_A | BTN_B | BTN_R | BTN_CUP | BTN_CLEFT | BTN_CRIGHT | BTN_CDOWN))) {
            this->timer = 0;
        }
    }
}

void ArmsHook_Update(Actor* thisx, GlobalContext* globalCtx) {
    ArmsHook* this = (ArmsHook*)thisx;

    this->actionFunc(this, globalCtx);
    this->unk_1F4 = this->unk_1E8;
}

void ArmsHook_Draw(Actor* thisx, GlobalContext* globalCtx) {
    s32 pad;
    ArmsHook* this = (ArmsHook*)thisx;
    Player* player = GET_PLAYER(globalCtx);
    Vec3f sp78;
    Vec3f hookNewTip;
    Vec3f hookNewBase;
    f32 sp5C;
    f32 sp58;

    if ((player->actor.draw != NULL) && (player->rightHandType == PLAYER_MODELTYPE_RH_HOOKSHOT)) {
        OPEN_DISPS(globalCtx->state.gfxCtx, "../z_arms_hook.c", 850);

        if ((ArmsHook_Shoot != this->actionFunc) || (this->timer <= 0)) {
            Matrix_MultVec3f(&D_80865B70, &this->unk_1E8);
            Matrix_MultVec3f(&D_80865B88, &hookNewTip);
            Matrix_MultVec3f(&D_80865B94, &hookNewBase);
            this->hookInfo.active = 0;
        } else {
            Matrix_MultVec3f(&D_80865B7C, &this->unk_1E8);
            Matrix_MultVec3f(&D_80865BA0, &hookNewTip);
            Matrix_MultVec3f(&D_80865BAC, &hookNewBase);
        }

        func_80090480(globalCtx, &this->collider, &this->hookInfo, &hookNewTip, &hookNewBase);
        func_80093D18(globalCtx->state.gfxCtx);
        gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(globalCtx->state.gfxCtx, "../z_arms_hook.c", 895),
                  G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_OPA_DISP++, gLinkAdultHookshotTipDL);
        Matrix_Translate(this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z, MTXMODE_NEW);
        Math_Vec3f_Diff(&player->unk_3C8, &this->actor.world.pos, &sp78);
        sp58 = SQ(sp78.x) + SQ(sp78.z);
        sp5C = sqrtf(sp58);
        Matrix_RotateY(Math_FAtan2F(sp78.x, sp78.z), MTXMODE_APPLY);
        Matrix_RotateX(Math_FAtan2F(-sp78.y, sp5C), MTXMODE_APPLY);
        Matrix_Scale(0.015f, 0.015f, sqrtf(SQ(sp78.y) + sp58) * 0.01f, MTXMODE_APPLY);
        gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(globalCtx->state.gfxCtx, "../z_arms_hook.c", 910),
                  G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_OPA_DISP++, gLinkAdultHookshotChainDL);

        CLOSE_DISPS(globalCtx->state.gfxCtx, "../z_arms_hook.c", 913);
    }
}
