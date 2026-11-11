void Player_UpdateBurningDekuStick(PlayState* play, Player* this) {
    f32 temp;

    if (GameInteractor_Should(VB_DEKU_STICK_BURN_OUT, this->unk_85C == 0.0f)) {
        Player_UseItem(play, this, ITEM_NONE);
        return;
    }

    temp = 1.0f;
    uint8_t vanillaShouldBurnOutCondition = DECR(this->unk_860) == 0;
    if (GameInteractor_Should(VB_DEKU_STICK_BURN_OUT, vanillaShouldBurnOutCondition)) {
        Inventory_ChangeAmmo(ITEM_STICK, -1);
        this->unk_860 = 1;
        temp = 0.0f;
        this->unk_85C = temp;
    } else if (this->unk_860 > 200) {
        temp = (210 - this->unk_860) / 10.0f;
    } else if (GameInteractor_Should(VB_DEKU_STICK_BURN_DOWN, this->unk_860 < 20)) {
        temp = this->unk_860 / 20.0f;
        this->unk_85C = temp;
    }

    func_8002836C(play, &this->meleeWeaponInfo[0].tip, &D_808547A4, &D_808547B0, &D_808547BC, &D_808547C0,
                  temp * 200.0f, 0, 8);
}
