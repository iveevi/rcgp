void sr()
{
    int lvar0;
    lvar0 = 12;
    int lvar1;
    lvar1 = 11;
    bool lvar2;
    lvar2 = (lvar0 < lvar1);
    int lvar3;
    lvar3 = 5;
    bool lvar4;
    lvar4 = (lvar0 > lvar3);
    bool lvar5;
    lvar5 = (lvar2 && lvar4);
    int lvar6;
    lvar6 = 11;
    bool lvar7;
    lvar7 = (lvar0 > lvar6);
    if (lvar7) {
        int lvar8;
        lvar8 = 1;
        lvar0 = (lvar0 + lvar8);
    }
    else if (lvar5) {
        int lvar9;
        lvar9 = 2;
        lvar0 = (lvar0 + lvar9);
    }
    else {
        int lvar10;
        lvar10 = 3;
        lvar0 = (lvar0 + lvar10);
    }
}
