void func_808442D8(PlayState* play, Player* this) {
    f32 var_fa0;
    f32 temp_fv1;

    if (this->unk_B0C == 0.0f) {
        Player_UseItem(play, this, ITEM_NONE);
        return;
    }

    var_fa0 = 1.0f;
    if (DECR(this->unk_B28) == 0) {
        Inventory_ChangeAmmo(ITEM_DEKU_STICK, -1);
        this->unk_B28 = 1;
        this->unk_B0C = 0.0f;
        var_fa0 = 0.0f;
    } else if (this->unk_B28 >= 0xC9) {
        var_fa0 = (0xD2 - this->unk_B28) / 10.0f;
    } else if (this->unk_B28 < 0x14) {
        var_fa0 = this->unk_B28 / 20.0f;
        this->unk_B0C = var_fa0;
    }

    if (var_fa0 > 0.0f) {
        func_800B0EB0(play, &this->meleeWeaponInfo[0].tip, &D_8085D364, &D_8085D370, &D_8085D37C, &D_8085D380,
                      (var_fa0 * 200.0f), 0, 8);
        if (play->roomCtx.curRoom.enablePosLights || (MREG(93) != 0)) {
            temp_fv1 = (Rand_ZeroOne() * 30.0f) + 225.0f;
            Lights_PointSetColorAndRadius(&this->lightInfo, temp_fv1, temp_fv1 * 0.7f, 0, var_fa0 * 300.0f);
        }
    }
}
