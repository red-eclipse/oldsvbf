GVAR(IDF_ADMIN, serverdebug, 0, 0, 3);
GVAR(IDF_ADMIN, serverclients, 1, 16, MAXCLIENTS);
GVAR(IDF_ADMIN, serveropen, 0, 3, 3);
GSVAR(IDF_ADMIN, serverdesc, "");
GSVAR(IDF_ADMIN, servermotd, "");

GVAR(IDF_ADMIN, modelimit, 0, G_LOBBY, G_MAX-1);
GVAR(IDF_ADMIN, mutslimit, 0, G_M_ALL, G_M_ALL);
GVAR(IDF_ADMIN, modelock, 0, 4, 5); // 0 = off, 1 = master only (+1 admin only), 3 = master can only set limited mode and higher (+1 admin), 5 = no mode selection
GVAR(IDF_ADMIN, mapslock, 0, 2, 5); // 0 = off, 1 = master can select non-allow maps (+1 admin), 3 = master can select non-rotation maps (+1 admin), 5 = no map selection
GVAR(IDF_ADMIN, varslock, 0, 1, 2); // 0 = master, 1 = admin only, 2 = nobody
GVAR(IDF_ADMIN, votelock, 0, 2, 5); // 0 = off, 1 = master can select same game (+1 admin), 3 = master only can vote (+1 admin), 5 = no voting
GVAR(IDF_ADMIN, votewait, 0, 3000, INT_MAX-1);

GVAR(IDF_ADMIN, resetmmonend, 0, 1, 2); // reset mastermode on end (1: just when empty, 2: when matches end)
GVAR(IDF_ADMIN, resetbansonend, 0, 1, 2); // reset bans on end (1: just when empty, 2: when matches end)
GVAR(IDF_ADMIN, resetvarsonend, 0, 1, 2); // reset variables on end (1: just when empty, 2: when matches end)

GVARF(IDF_ADMIN, gamespeed, 1, 100, 1000, timescale = sv_gamespeed, timescale = gamespeed);
GVARF(IDF_ADMIN, gamepaused, 0, 0, 1, paused = sv_gamepaused, paused = gamepaused);

GSVAR(IDF_ADMIN, defaultmap, "");
GVAR(IDF_ADMIN, defaultmode, -1, G_DEATHMATCH, G_MAX-1);
GVAR(IDF_ADMIN, defaultmuts, -2, G_M_TEAM, G_M_ALL);
GVAR(IDF_ADMIN, storyplayers, 1, 5, MAXPLAYERS);

GSVAR(IDF_ADMIN, allowmaps, "bath bloodgrounds chaos citadel darkness deadsimple deathtrap deli dualstar dutility eight enigma firehouse forge gladiator hollow longestyard mirage nova oasis overseer panic refuge rivals siege smouldering testchamber tower tranquility venus warehouse warground wet wishbone");
GSVAR(IDF_ADMIN, mainmaps, "bath bloodgrounds citadel darkness deadsimple deathtrap deli eight enigma gladiator longestyard mirage nova oasis panic refuge rivals smouldering tower warground venus");
GSVAR(IDF_ADMIN, duelmaps, "bath citadel darkness deadsimple deathtrap dutility eight gladiator longestyard nova panic refuge rivals tower warground venus");
GSVAR(IDF_ADMIN, ctfmaps, "bath citadel darkness deadsimple deli enigma forge gladiator mirage nova oasis panic refuge rivals smouldering warground venus");
GSVAR(IDF_ADMIN, mctfmaps, "deadsimple enigma oasis refuge");
GSVAR(IDF_ADMIN, stfmaps, "bath bloodgrounds citadel darkness deadsimple deli enigma forge gladiator mirage nova oasis panic refuge rivals smouldering tower warground venus");
GSVAR(IDF_ADMIN, trialmaps, "testchamber");
GSVAR(IDF_ADMIN, storymaps, "wishbone storytest");
GVAR(0, maprotate, 0, 2, 2); // 0 = off, 1 = sequence, 2 = random

GVAR(0, maxcarry, 1, 2, WEAP_MAX-1);
GVAR(0, spawnrotate, 0, 2, 2); // 0 = let client decide, 1 = sequence, 2 = random
GVAR(0, spawnweapon, 0, WEAP_PISTOL, WEAP_TOTAL-1);
GVAR(0, instaweapon, 0, WEAP_INSTA, WEAP_TOTAL-1);
GVAR(0, trialweapon, 0, WEAP_MELEE, WEAP_TOTAL-1);
GVAR(0, spawngrenades, 0, 0, 2); // 0 = never, 1 = all but instagib/time-trial, 2 = always
GVAR(0, spawndelay, 0, 5000, INT_MAX-1); // delay before spawning in most modes
GVAR(0, instadelay, 0, 2500, INT_MAX-1); // .. in instagib/arena matches
GVAR(0, trialdelay, 0, 1000, INT_MAX-1); // .. in time trial matches
GVAR(0, spawnprotect, 0, 3000, INT_MAX-1); // delay before damage can be dealt to spawning player
GVAR(0, instaprotect, 0, 1500, INT_MAX-1); // .. in instagib/arena matches

GVAR(0, maxhealth, 0, 100, INT_MAX-1);
GVAR(0, extrahealth, 0, 100, INT_MAX-1);

GVAR(0, fireburntime, 0, 5500, INT_MAX-1);
GVAR(0, fireburndelay, 0, 1000, INT_MAX-1);
GVAR(0, fireburndamage, 0, 7, INT_MAX-1);

GVAR(0, vampire, 0, 0, 1);
GVAR(0, regendelay, 0, 3000, INT_MAX-1);
GVAR(0, regenguard, 0, 1000, INT_MAX-1);
GVAR(0, regentime, 0, 1000, INT_MAX-1);
GVAR(0, regenhealth, 0, 5, INT_MAX-1);
GVAR(0, regenextra, 0, 10, INT_MAX-1);
GVAR(0, regenflag, 0, 1, 2); // 0 = off, 1 = only guarding, 2 = also while carrying

GVAR(0, itemsallowed, 0, 2, 2); // 0 = never, 1 = all but instagib/time-trial, 2 = always
GVAR(0, itemdropping, 0, 1, 1); // 0 = never, 1 = yes
GVAR(0, itemspawntime, 1, 15000, INT_MAX-1); // when items respawn
GVAR(0, itemspawndelay, 0, 1000, INT_MAX-1); // after map start items first spawn
GVAR(0, itemspawnstyle, 0, 1, 2); // 0 = all at once, 1 = staggered, 2 = random
GFVAR(0, itemthreshold, 0, 1, 1000); // if numitems/numclients/maxcarry is less than this, spawn one of this type
GVAR(0, kamikaze, 0, 1, 3); // 0 = never, 1 = holding grenade, 2 = have grenade, 3 = always

GVAR(0, timelimit, 0, 15, INT_MAX-1);
GVAR(0, triallimit, 0, 60000, INT_MAX-1);
GVAR(0, intermlimit, 0, 15000, INT_MAX-1); // .. before vote menu comes up
GVAR(0, votelimit, 0, 30000, INT_MAX-1); // .. before vote passes by default
GVAR(0, duelreset, 0, 1, 1); // reset winner in duel
GVAR(0, duelclear, 0, 1, 1); // clear items in duel
GVAR(0, duellimit, 0, 5000, INT_MAX-1); // .. before duel goes to next round

GVAR(0, selfdamage, 0, 1, 1); // 0 = off, 1 = either hurt self or use teamdamage rules
GVAR(0, trialdamage, 0, 0, 1); // 0 = off, 1 = allow damage in time-trial
GVAR(0, teamdamage, 0, 1, 2); // 0 = off, 1 = non-bots damage team, 2 = all players damage team
GVAR(0, teambalance, 0, 1, 3); // 0 = off, 1 = by number then rank, 2 = by rank then number, 3 = humans vs. ai

GVAR(0, fraglimit, 0, 0, INT_MAX-1); // finish when score is this or more

GVAR(0, ctflimit, 0, 0, INT_MAX-1); // finish when score is this or more
GVAR(0, ctfstyle, 0, 0, 3); // 0 = classic touch-and-return, 1 = grab and take home, 2 = defend and reset, 3 = dominate and protect
GVAR(0, ctfresetdelay, 0, 30000, INT_MAX-1);

GVAR(0, stflimit, 0, 0, INT_MAX-1); // finish when score is this or more
GVAR(0, stfstyle, 0, 1, 1); // 0 = overthrow and secure, 1 = instant secure
GVAR(0, stffinish, 0, 0, 1); // finish when all bases captured
GVAR(0, stfpoints, 0, 1, INT_MAX-1); // points added to score
GVAR(0, stfoccupy, 0, 100, INT_MAX-1); // points needed to occupy

GVAR(0, botbalance, -1, -1, MAXAI/2); // -1 = populate bots to map defined numplayers, 0 = don't balance, 1 or more = fill only with this*numteams
GVAR(0, botminskill, 1, 70, 101);
GVAR(0, botmaxskill, 1, 80, 101);
GVAR(0, botlimit, 0, 16, MAXAI/2);

GFVAR(0, forcegravity, -1, -1, 1000);
GFVAR(0, forcejumpspeed, -1, -1, 1000);
GFVAR(0, forcemovespeed, -1, -1, 1000);
GFVAR(0, forcemovecrawl, -1, -1, 1000);
GFVAR(0, forceimpulsespeed, -1, -1, 1000);

GVAR(0, forceimpulsestyle, -1, -1, 3);
GVAR(0, forceimpulsemeter, -1, -1, INT_MAX-1);
GVAR(0, forceimpulsecost, -1, -1, INT_MAX-1);
GVAR(0, forceimpulsecount, -1, -1, INT_MAX-1);
GVAR(0, forceimpulseskate, -1, -1, INT_MAX-1);
GFVAR(0, forceimpulseregen, -1, -1, 1000);

GFVAR(0, forceliquidspeed, -1, -1, 1);
GFVAR(0, forceliquidcurb, -1, -1, 1000);
GFVAR(0, forcefloorcurb, -1, -1, 1000);
GFVAR(0, forceaircurb, -1, -1, 1000);

GFVAR(0, damagescale, -1000, 1, 1000);
GFVAR(0, hitpushscale, -1000, 1, 1000);
GFVAR(0, deadpushscale, -1000, 2, 1000);

GFVAR(0, wavepusharea, 0, 2, 1000);
GFVAR(0, wavepushscale, 0, 1, 1000);

GVAR(0, multikilldelay, 0, 5000, INT_MAX-1);
GVAR(0, spreecount, 0, 5, INT_MAX-1);
GVAR(0, dominatecount, 0, 5, INT_MAX-1);
