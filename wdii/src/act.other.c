/* ************************************************************************
*   File: act.other.c                                   Part of CircleMUD *
*  Usage: Miscellaneous player-level commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __ACT_OTHER_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "dg_scripts.h"
#include "buffer.h"

/* extern variables */
extern struct con_app_type con_app[];
extern struct str_app_type str_app[];
extern struct room_data *world;
extern struct index_data *obj_index;
extern struct obj_data *obj_proto;
extern struct descriptor_data *descriptor_list;
extern struct dex_skill_type dex_app_skill[];
extern struct spell_info_type spell_info[];
extern const char *dirs[];
extern struct index_data *mob_index;
extern char *class_abbrevs[];
extern int free_rent;
extern int pt_allowed;
extern int max_filesize;
extern int nameserver_is_slow;
extern int top_of_world;
extern int auto_save;
extern char *npc_class_abbrevs[];
extern char *spells[];
char *item_condition(struct obj_data *obj);
/* extern procedures */
void appear(struct char_data * ch);
void perform_immort_vis(struct char_data *ch);
SPECIAL(shop_keeper);
ACMD(do_gen_comm);
void die(struct char_data * ch, struct char_data * killer);
void Crash_rentsave(struct char_data * ch, int cost);
void write_aliases(struct char_data *ch);
void extract_char_to_quit(struct char_data * ch);
int can_pk(struct char_data * ch, struct char_data * vt);
extern int siteok_everyone;
void play_sound(struct char_data *ch, char *sound, int type);
extern int top_of_p_table;
extern struct player_index_element *player_table;
/* local functions */


ACMD(do_craft);
ACMD(do_brew);
ACMD(do_quit);
ACMD(do_save);
ACMD(do_not_here);
ACMD(do_aid);
ACMD(do_sneak);
ACMD(do_hide);
ACMD(do_steal);
ACMD(do_practice);
ACMD(do_visible);
int perform_group(struct char_data *ch, struct char_data *vict);
void print_group(struct char_data *ch);
ACMD(do_group);
ACMD(do_ungroup);
ACMD(do_report);
ACMD(do_split);
ACMD(do_use);
ACMD(do_wimpy);
ACMD(do_display);
ACMD(do_gen_write);
ACMD(do_gen_tog);
ACMD(do_sacrifice);
ACMD(do_title);
ACMD(do_meditate);
ACMD(do_listen);
ACMD(do_transform);
ACMD(do_blood_ritual);
ACMD(do_recharge);
ACMD(do_message);
ACMD(do_notips);
ACMD(do_nstart);
ACMD(do_changeicq);
ACMD(do_namechange);	

		
ACMD(do_nstart)
{
	 struct obj_data *obj;
  int ii;
  int give_obj[] = { 3051, 3052, 2544, 18606, 3814, 3814, 18604, 18628, 18626, 18624, 18623, 18621, 18621, 18619, 18612, 18613, 18607, 18607, 18609, 18603,18605, 18605,18602, -1 };

  
  if ((GET_LEVEL(ch) < 10) && (GET_REMORT(ch) == 0) && (GET_LEVEL(ch) > 1))
  {

  if (siteok_everyone) 
    SET_BIT(PLR_FLAGS(ch), PLR_SITEOK);

  SET_BIT(PRF_FLAGS(ch), PRF_AUTOEXIT);
    
  GET_EXP(ch) = 1;
  GET_ALIGNMENT(ch) = 5 ; 

  GET_GOLD(ch) += 5000;
  
   GET_POINTS(ch) = 90 - (GET_LEVEL(ch)*9);
   GET_POINTS_S(ch) = 90 - (GET_LEVEL(ch)*9);
  GET_LEVEL(ch) = 10;
  set_title(ch, NULL);
  set_prename(ch, NULL);
  roll_real_abils(ch);

  
  SET_BIT(PRF_FLAGS(ch), PRF_DISPHP);
  SET_BIT(PRF_FLAGS(ch), PRF_DISPMOVE);
  SET_BIT(PRF2_FLAGS(ch), PRF2_DISPEXP);
  SET_BIT(PLR_FLAGS(ch), PLR_NOPK);

  for(ii = 0; give_obj[ii] != -1; ii++) {
   obj = read_object(give_obj[ii], VIRTUAL);
    if (obj == NULL)
     continue;
    obj_to_char(obj, ch);
  }

  switch (GET_CLASS(ch)) {
   case CLASS_MAGIC_USER:
   case CLASS_CLERIC:
   case CLASS_NECROMANCER:
   case CLASS_PSIONICIST:
   case CLASS_WARLOCK:
   case CLASS_SORCERER:
   case CLASS_PALADIN:
     SET_BIT(PRF_FLAGS(ch), PRF_DISPMANA);
     break;
   case CLASS_THIEF:
     break;
   case CLASS_WARRIOR:
     break;
   case CLASS_NINJA:
     break;
   case CLASS_BARBARIAN:
     break;
   case CLASS_RANGER:
     break;

  }

  advance_level(ch);

  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);
  GET_MENTAL(ch) = GET_MAX_MENTAL(ch);
  GET_OXI(ch) = GET_MAX_OXI(ch);

  GET_COND(ch, THIRST) = 24;
  GET_COND(ch, FULL) = 24;
  GET_COND(ch, DRUNK) = 0;

  ch->player.time.played = 0;
  ch->player.time.logon = time(0);
}
else 
send_to_char("&RYou have no level to do this.&n\r\n", ch);
return;
}

ACMD(do_quit)
{
  struct descriptor_data *d, *next_d;

  if (IS_NPC(ch) || !ch->desc)
    return;

  if (subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_IMMORT)
    send_to_char("You have to type quit--no less, to quit!\r\n", ch);
  else if (GET_POS(ch) == POS_FIGHTING)
    send_to_char("No way!  You're fighting for your life!\r\n", ch);
  else if (ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
    send_to_char("Are you trying to get out the WAR? Looser.\r\n", ch);
  else if (GET_LEVEL(ch) < LVL_ELDER && !PLR_FLAGGED(ch, PLR_NOPK) && !ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL))
    send_to_char("&RPlayer killers&W can &GQUIT&W only in &CPEACEFUL&W rooms.&n\r\n", ch);
  else if (GET_POS(ch) < POS_STUNNED) {
    send_to_char("You die before your time...\r\n", ch);
    die(ch, NULL);
  } else {

    if (!GET_INVIS_LEV(ch))
      act("$n leaves from the WarDome.", TRUE, ch, 0, 0, TO_ROOM);
    sprintf(buf, "%s has quit the game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);

    /*
     * kill off all sockets connected to the same player as the one who is
     * trying to quit.  Helps to maintain sanity as well as prevent duping.
     */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (d == ch->desc)
        continue;
      if (d->character && (GET_IDNUM(d->character) == GET_IDNUM(ch)))
        STATE(d) = CON_DISCONNECT;
       play_sound(ch, "EXIT.wav", SND_CHAR);
    }

   save_char(ch, ch->in_room);
   write_aliases(ch);
   if (free_rent)
      Crash_rentsave(ch, 0);
/*   sprintf(buf, "\r\n&CA strange voice whispers, 'The WarDome will wait for you, &c%s&C...'&n\r\n", GET_NAME(ch));
   send_to_char(buf, ch);
   make_summary(ch);*/
   extract_char(ch);           /* Char is saved in extract char */
  }
}



ACMD(do_save)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  /* Only tell the char we're saving if they actually typed "save" */
  if (cmd) {
    /*
     * This prevents item duplication by two PC's using coordinated saves
     * (or one PC with a house) and system crashes. Note that houses are
     * still automatically saved without this enabled.
     */
    if (auto_save) {
      send_to_char("Only the WarDome can save your character.\r\n", ch);
      return;
    }
    sprintf(buf, "Saving %s.\r\n", GET_NAME(ch));
    send_to_char(buf, ch);
  }

  write_aliases(ch);
  save_char(ch, ch->in_room);
  Crash_crashsave(ch);
  if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE_CRASH))
    House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));
}


/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char("Sorry, but you cannot do that here!\r\n", ch);
}

ACMD(do_aid)
{
  struct char_data *victim;
  char victim_name[240];

  one_argument(argument, victim_name);

  if (!GET_SKILL(ch, SKILL_FIRST_AID)) {
    send_to_char("You don't know how!\r\n", ch);
    return;
  }

  if (!(victim = get_char_room_vis(ch, victim_name))) {
    send_to_char("Can't aid someone not here!\r\n", ch);
    return;
  }

  if (number(1, 200) < GET_SKILL(ch, SKILL_FIRST_AID)) {
    GET_HIT(victim) += number (1, 4);
    update_pos(victim);
    send_to_char("You succeed to stop some of the bleeding.\r\n", ch);
    act("$n bandages your wounds.", TRUE, ch, 0, victim, TO_VICT);
    improve_skill(ch, SKILL_FIRST_AID);
  } else {
    send_to_char("You can't stop the bleeding!  Panic!\r\n", ch);
  }

  WAIT_STATE(ch, PULSE_VIOLENCE * 1);
}

void warshout(struct char_data *tch)
{
  struct affected_type af;

    if(affected_by_spell(tch, SKILL_WARSHOUT)) {
      affect_from_char(tch, SKILL_WARSHOUT);
      return;
    }

    af.type = SKILL_WARSHOUT;
    af.location = APPLY_DAMROLL;
    af.duration = 1;
    af.bitvector = 0;
    af.bitvector2 = AFF2_WARSHOUT;
    af.bitvector3 = 0;
    af.modifier = 5;
    affect_to_char(tch, &af);

}

int mandar_warshout(struct char_data * ch)
{
  struct char_data *tch, *k;
  struct follow_type *f, *f_next;
  bool i = 0;

  if (ch == NULL)
    return (0);

  if (!AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char("You need to be in a group to shout.\r\n", ch);
    return (0);
  }
  if (ch->master != NULL) {
    send_to_char("You need to be the group leader to shout.\r\n", ch);
    return (0);
  } else
    k = ch;
  for (f = k->followers; f; f = f_next) {
    f_next = f->next;
    tch = f->follower;
    if (tch->in_room != ch->in_room)
      continue;
    if (!AFF_FLAGGED(tch, AFF_GROUP))
      continue;
    if (AFF2_FLAGGED(tch, AFF2_TRANSFORM))
      continue;
    if (ch == tch)
      continue;
    warshout(tch);
  }

  if(!i) {
     if ((k != ch) && AFF_FLAGGED(k, AFF_GROUP))
           warshout(k);
     warshout(ch);
  } else {
     send_to_char("Alone in a group? Is not good.\r\n", ch);
     return (0);
  }

  return (1);
}

/* terminar */
ACMD(do_warshout)
{
  if (!GET_SKILL(ch, SKILL_WARSHOUT)){
    send_to_char("You have no idea how.\r\n", ch);
    return;
  }

  if(GET_MOVE(ch) < 80)
  {
   send_to_char("You have no move.\r\n", ch) ;
   return ;
  }

  if(mandar_warshout(ch)) {
	  GET_MOVE(ch) -= 75;
  	  send_to_char("&RYou yell, 'WWWWWWAAAAAAAAAAAARRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR!'&n\r\n",ch);
  	  act("&R$n yells, 'WWWWWWAAAAAAAAAAAARRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR!'&n", FALSE, ch, 0, 0, TO_ROOM);
	  improve_skill(ch, SKILL_WARSHOUT);
  }
}
ACMD(do_brew)
	{
	struct obj_data *mag_item;
	struct obj_data *obj;
 	int i;
  	
  
  half_chop(argument, arg, buf);

	if (!*arg){
	send_to_char("Brew what?\r\n", ch);
	return;
}

	if (!GET_SKILL(ch, SKILL_BREW)){
    send_to_char("You have no idea how.\r\n", ch);
    return;
  }
        if (!(mag_item = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
    return;
  }
   if ((GET_OBJ_TYPE(mag_item) != ITEM_ERB)){
   	send_to_char("You can just do potions with herbs.\r\n", ch);
   	return;
}
    if (GET_REMORT(ch) < 1){
    	i = (GET_LEVEL(ch) * 5);
	}
	if (GET_REMORT(ch) >= 1){ 
	i = ((GET_LEVEL(ch) * 5) + (GET_REMORT(ch) * 800));
	}
	if  (GET_MANA(ch) < i){
	send_to_char("You don't have enough mana to mix"
			     " that potion!\r\n", ch);
		return;
	}
    
      act("&wYou mix &W$p &wand create a potion from it.&n", FALSE, ch, mag_item, NULL, TO_CHAR);
    if (mag_item->action_description)
      act(mag_item->action_description, FALSE, ch, mag_item, NULL, TO_ROOM);
    else
      act("&w$n &Wmixes&w $p &Wand creates a potion from it.&n", TRUE, ch, mag_item, NULL, TO_ROOM);
	
	 if ((number(1, 3) == 3) && (GET_LEVEL(ch) < LVL_IMMORT)) {
		send_to_char("&RYAs you begin mixing the potion, it violently"
			     " explodes!&n\r\n",ch);
		act("&R$n begins to mix a potion, but it suddenly explodes!&n",
		    FALSE, ch, 0,0, TO_ROOM);
		extract_obj(mag_item);
	}
    	
    	 GET_MANA(ch) -= i;
    	 obj = read_object(525, VIRTUAL);
    	 GET_OBJ_VAL(obj, 1) = GET_OBJ_VAL(mag_item, 1) ;
    	 GET_OBJ_WEIGHT(obj) = 1 ;
	  GET_OBJ_VAL(obj, 2) = -1; 
	  GET_OBJ_VAL(obj, 3) = -1;  
	 // GET_OBJ_EXTRA(obj) = ITEM_NORENT;
	  GET_OBJ_LEVEL(obj) = (GET_LEVEL(ch)); 
	 // GET_OBJ_RENT(obj) = 0;
	  obj_to_char(obj, ch);
	  extract_obj(mag_item);	 
    	
      
      improve_skill(ch, SKILL_BREW);
  	

}
ACMD(do_sneak)
{
  struct affected_type af;
  byte percent;

  if (!GET_SKILL(ch, SKILL_SNEAK)){
    send_to_char("You have no idea how.\r\n", ch);
    return;
  }

  send_to_char("Okay, you'll try to move silently for a while.\r\n", ch);
  if (AFF_FLAGGED(ch, AFF_SNEAK)){
    send_to_char("You are already sneaking.\r\n", ch);
    return;
  }

  percent = number(1, 101);     /* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_SNEAK) + (15 + 
con_app[GET_DEX(ch)].hitp))
    return;

  improve_skill(ch, SKILL_SNEAK);

  af.type = SKILL_SNEAK;
  af.duration = GET_LEVEL(ch)/6;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_SNEAK;
  af.bitvector2 = 0;
  af.bitvector3 = 0;
  affect_to_char(ch, &af);

}

ACMD(do_hide)
{
  byte percent;

  if (!GET_SKILL(ch, SKILL_HIDE)){
    send_to_char("You have no idea how.\r\n", ch);
    return;
  }

  send_to_char("You attempt to hide yourself.\r\n", ch);

  if (AFF_FLAGGED(ch, AFF_HIDE))
    REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);

  percent = number(1, 101);     /* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_HIDE) + (15 + 
con_app[GET_DEX(ch)].hitp))
    return;

  send_to_char("You are hide on the room now.\r\n", ch);
  SET_BIT(AFF_FLAGS(ch), AFF_HIDE);
  improve_skill(ch, SKILL_HIDE);
}

ACMD(do_steal)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
  int percent, gold, eq_pos, pcsteal = 0, ohoh = 0;

  if (!GET_SKILL(ch, SKILL_STEAL)){
    send_to_char("You have no idea how.\r\n", ch);
    return;
  }

   if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
	   send_to_char("This room just has such a peaceful, easy feeling...\r\n", ch);
	   return;
   }

   if (world[ch->in_room].sector_type == ROOM_ARENA) {
	   send_to_char("You can't steal in the battle fields.\r\n", ch);
	   return;
   }

  argument = one_argument(argument, obj_name);
  one_argument(argument, vict_name);

  if (!(vict = get_char_room_vis(ch, vict_name))) {
    send_to_char("Steal what from who?\r\n", ch);
    return;
  } else if (vict == ch) {
    send_to_char("Come on now, that's rather stupid!\r\n", ch);
    return;
  }

  if(!IS_NPC(vict))
  {
   if(!can_pk(ch,vict))
   {
    send_to_char("This person is out of your PK range.\r\n", ch);
    return ;
   }
  }

  if (MOB_FLAGGED(vict, MOB_NOSTEAL))
  {
	  send_to_char("This mob can not be stolen.\r\n", ch);
	  return;
  }

  /* 101% is a complete failure */
  percent = number(1, 110) - (15 + con_app[GET_DEX(ch)].hitp);

  if (GET_POS(vict) < POS_SLEEPING)
    percent = -1;               /* ALWAYS SUCCESS */

  /* NO NO With Imp's and Shopkeepers, and if player thieving is not allowed */
  if (GET_LEVEL(vict) >= LVL_IMMORT || pcsteal ||
      GET_MOB_SPEC(vict) == shop_keeper)
    percent = 101;              /* Failure */

  if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold")) {

    if (!(obj = get_obj_in_list_vis(vict, obj_name, vict->carrying))) {

      for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
        if (GET_EQ(vict, eq_pos) &&
            (isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
            CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
          obj = GET_EQ(vict, eq_pos);
          break;
        }
      if (!obj) {
      	sprintf(buf,"%s hasn't got that item.\r\n",HSSH(ch));
        send_to_char(buf, ch);
        return;
      } else {                  /* It is equipment */
        if ((GET_POS(vict) > POS_SLEEPING)) {
          send_to_char("Steal the equipment now?  Impossible!\r\n", ch);
          return;
        } else {
          sprintf(buf,"You unequip %s and steal it.\r\n",(obj)->short_description);
          send_to_char(buf, ch);
          act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
          obj_to_char(unequip_char(vict, eq_pos), ch);
	  improve_skill(ch, SKILL_STEAL);
//	  SET_BIT(PLR_FLAGS(ch), PLR_THIEF);
        }
      }
    } else {                    /* obj found in inventory */

      percent += GET_OBJ_WEIGHT(obj);   /* Make heavy harder */

      if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
        ohoh = TRUE;
        send_to_char("Oops..\r\n",ch);
        act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
        act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
      } else {                  /* Steal the item */
        if ((IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))) {
          if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) < CAN_CARRY_W(ch)) {
            obj_from_char(obj);
            obj_to_char(obj, ch);
            send_to_char("Got it!\r\n", ch);
	    improve_skill(ch, SKILL_STEAL);
//	    SET_BIT(PLR_FLAGS(ch), PLR_THIEF);
          }
        } else
          send_to_char("You cannot carry that much.\r\n", ch);
      }
    }
  } else {                      /* Steal some coins */
    if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
      ohoh = TRUE;
      send_to_char("Oops..\r\n", ch);
      act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, vict, TO_VICT);
      act("$n tries to steal gold from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
    } else {
      /* Steal some gold coins */
      gold = (int) ((GET_GOLD(vict) * number(1, 10)) / 100);
      gold = MIN(1782, gold);
      if (gold > 0) {
        GET_GOLD(ch) += gold;
	GET_GOLD_SUM(ch) += gold;
        GET_GOLD(vict) -= gold;
        improve_skill(ch, SKILL_STEAL);
        if (gold > 1) {
          sprintf(buf, "Bingo!  You got %d gold coins.\r\n", gold);
          send_to_char(buf, ch);
        } else {
          send_to_char("You manage to swipe a solitary gold coin.\r\n", ch);
        }
      } else {
        send_to_char("You couldn't get any gold...\r\n", ch);
      }
    }
  }

  if(ohoh && !IS_NPC(vict))
   SET_BIT(PLR_FLAGS(ch), PLR_THIEF);

  if (ohoh && IS_NPC(vict) && AWAKE(vict))
    hit(vict, ch, TYPE_UNDEFINED);
}



ACMD(do_visible)
{
  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    perform_immort_vis(ch);
    return;
  }

  if AFF_FLAGGED(ch, AFF_INVISIBLE) {
    appear(ch);
    send_to_char("You break the spell of invisibility.\r\n", ch);
  } else
    send_to_char("You are already visible.\r\n", ch);
}



ACMD(do_title)
{
  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (IS_NPC(ch))
    send_to_char("Your title is fine... go away.\r\n", ch);
  else if (PLR_FLAGGED(ch, PLR_NOTITLE))
    send_to_char("You can't title yourself -- you shouldn't have abused it!\r\n", ch);
  else if (strstr(argument, "{") || strstr(argument, "}"))
    send_to_char("Titles can't contain the { or } characters.\r\n", ch);
  else if (strstr(argument, "[") || strstr(argument, "]"))
    send_to_char("Titles can't contain the [ or ] characters.\r\n", ch);
  else if (strstr(argument, "(") || strstr(argument, ")"))
    send_to_char("Titles can't contain the ( or ) characters.\r\n", ch);
  else if (strlen(argument) > (MAX_TITLE_LENGTH - 1))  {
    sprintf(buf, "Sorry, titles can't be longer than %d characters.\r\n",
            (MAX_TITLE_LENGTH - 1));
    send_to_char(buf, ch);
  }
  else {
    set_title(ch, (!*argument ? "the wardome player" : argument));
    sprintf(buf, "Okay, you're now %s%s %s.\r\n",
    (GET_PRENAME(ch) != NULL ? GET_PRENAME(ch) : ""),
    GET_NAME(ch), GET_TITLE(ch));
    send_to_char(buf, ch);
  }
}

ACMD(do_prename)
{
  char argbkp[MAX_TITLE_LENGTH];

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (IS_NPC(ch))
    send_to_char("Your prename is fine... go away.\r\n", ch);
  else if (GET_LEVEL(ch) < LVL_ELDER && GET_REMORT(ch) < 1)
    send_to_char("You need be more experienced to set your prename.\r\n", ch);
  else if (strstr(argument, " "))
    send_to_char("Titles can't contain spaces. \r\n", ch);
  else if (PLR_FLAGGED(ch, PLR_NOTITLE))
    send_to_char("You can't title yourself -- you shouldn't have abused it!\r\n", ch);
  else if (strstr(argument, "{") || strstr(argument, "}"))
    send_to_char("Titles can't contain the { or } characters.\r\n", ch);
  else if (strstr(argument, "[") || strstr(argument, "]"))
    send_to_char("Titles can't contain the [ or ] characters.\r\n", ch);
  else if (strstr(argument, "(") || strstr(argument, ")"))
    send_to_char("Titles can't contain the ( or ) characters.\r\n", ch);
  else if (strlen(argument) > MAX_TITLE_LENGTH) {
    sprintf(buf, "Sorry, prenames can't be longer than %d characters.\r\n",
            MAX_TITLE_LENGTH);
    send_to_char(buf, ch);
  } else {
   if(!*argument) {
    set_prename(ch, NULL);
    sprintf(buf, "Okay, you're now %s %s.\r\n", GET_NAME(ch), GET_TITLE(ch));
    send_to_char(buf, ch);
   } else {
    strcpy(argbkp, argument);
    sprintf(argument, "%s&n ", argbkp);
    set_prename(ch, argument);
    sprintf(buf, "Okay, you're now %s%s %s.\r\n", GET_PRENAME(ch), GET_NAME(ch), GET_TITLE(ch));
    send_to_char(buf, ch);
   }
  }
}

int perform_group(struct char_data *ch, struct char_data *vict)
{
  if (AFF_FLAGGED(vict, AFF_GROUP) || !CAN_SEE(ch, vict))
    return (0);

  SET_BIT(AFF_FLAGS(vict), AFF_GROUP);
  if (ch != vict)
    act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
  act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
  act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);
  return (1);
}


void print_group(struct char_data *ch)
{
  struct char_data *k;
  struct follow_type *f;

  if (!AFF_FLAGGED(ch, AFF_GROUP))
    send_to_char("But you are not the member of a group!\r\n", ch);
  else {
    send_to_char("&WYour &ggroup&W consists of:&n\r\n", ch);

    k = (ch->master ? ch->master : ch);

    if (AFF_FLAGGED(k, AFF_GROUP)) {
      sprintf(buf, "     &n[&W%9d &RHp &W%9d &MMn &W%9d &GMv&n] &C[&n%4d %s&C] &g$N &M(Captain)&n", GET_HIT(k),
	      GET_MANA(k), GET_MOVE(k), GET_LEVEL(k), CLASS_ABBR(k));
      act(buf, FALSE, ch, 0, k, TO_CHAR);
    }

    for (f = k->followers; f; f = f->next) {
      if (!AFF_FLAGGED(f->follower, AFF_GROUP))
	continue;

      sprintf(buf, "     &n[&W%9d &RHp &W%9d &MMn &W%9d &GMv&n] &C[&n%4d %s&C] &g$N&n", GET_HIT(f->follower),
	      GET_MANA(f->follower), GET_MOVE(f->follower),
	      GET_LEVEL(f->follower), CLASS_ABBR(f->follower));
      act(buf, FALSE, ch, 0, f->follower, TO_CHAR);
    }
  }
}

ACMD(do_group)
{
  struct char_data *vict;
  struct follow_type *f;
  int found;

  one_argument(argument, buf);

  if (!*buf) {
    print_group(ch);
    return;
  }

  if (ch->master) {
    send_to_char("You can not enroll group members without being head of a group.\r\n", ch);
    return;
  }

  if (!str_cmp(buf, "all")) {
	for (found = 0, f = ch->followers; f; f = f->next)
		if (can_pk(ch, f->follower)) {
			if(!IS_NPC(f->follower))
				found += perform_group(ch, f->follower);
      		} else {
			sprintf(buf,"&W%s &wis don't have level to enter in your group.&n\r\n",PERS(f->follower, ch));
        		send_to_char(buf, ch);
        		act("You don't have level to enter in the &W$n's &wgroup.&n", FALSE, ch, 0, f->follower, TO_VICT);
      		}
		if (!found)
      			send_to_char("You need someone following you to perform a group.\r\n", ch);
    		else
      			perform_group(ch, ch);
    		return;
  }

  if (!(vict = get_char_room_vis(ch, buf)))
    send_to_char(NOPERSON, ch);
  else if (ch == vict)
    send_to_char("Use 'group all' to create a group.\r\n", ch);
  else if (IS_NPC(vict))
    send_to_char("MOBs grouping?\r\n", ch);
  else if ((vict->master != ch) && (vict != ch)) {
    sprintf(buf,"%s must follow you to enter your group.\r\n",PERS(vict, ch));
    send_to_char(buf, ch);
    }
  else {
    if (!AFF_FLAGGED(vict, AFF_GROUP))
      if (((ch->player.level - vict->player.level) <= 20) && ((ch->player.level - vict->player.level) >= -20)){
       perform_group(ch, ch);
       perform_group(ch, vict);
      }
      else{
        sprintf(buf,"&W%s &wis don't have level to enter in your group.&n\r\n",PERS(vict, ch));
        send_to_char(buf, ch);
        act("You don't have level to enter in the &W$n's &wgroup.&n", FALSE, ch, 0, vict, TO_VICT);
      }
    else {
      if (ch != vict)
        sprintf(buf,"%s is no longer a member of your group.\r\n",PERS(vict, ch));
        send_to_char(buf, ch);
      act("You have been kicked out of $n's group!", FALSE, ch, 0, vict, TO_VICT);
      act("$N has been kicked out of $n's group!", FALSE, ch, 0, vict, TO_NOTVICT);
      REMOVE_BIT(AFF_FLAGS(vict), AFF_GROUP);
    }
  }
}



ACMD(do_ungroup)
{
  struct follow_type *f, *next_fol;
  struct char_data *tch;

  one_argument(argument, buf);

  if (!*buf) {
    if (ch->master || !(AFF_FLAGGED(ch, AFF_GROUP))) {
      send_to_char("But you lead no group!\r\n", ch);
      return;
    }
    sprintf(buf2, "%s has disbanded the group.\r\n", GET_NAME(ch));
    for (f = ch->followers; f; f = next_fol) {
      next_fol = f->next;
      if (AFF_FLAGGED(f->follower, AFF_GROUP)) {
        REMOVE_BIT(AFF_FLAGS(f->follower), AFF_GROUP);
        send_to_char(buf2, f->follower);
        if (!AFF_FLAGGED(f->follower, AFF_CHARM))
          stop_follower(f->follower);
      }
    }

    REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
    send_to_char("&wYou disband the group.&n\r\n", ch);
    return;
  }
  if (!(tch = get_char_room_vis(ch, buf))) {
    send_to_char("There is no such person!\r\n", ch);
    return;
  }
  if (tch->master != ch) {
    send_to_char("That person is not following you!\r\n", ch);
    return;
  }

  if (!AFF_FLAGGED(tch, AFF_GROUP)) {
    send_to_char("That person isn't in your group.\r\n", ch);
    return;
  }

  REMOVE_BIT(AFF_FLAGS(tch), AFF_GROUP);


  sprintf(buf,"%s is no longer a member of your group.\r\n",PERS(tch, ch));
  send_to_char(buf, ch);
  act("You have been kicked out of $n's group!", FALSE, ch, 0, tch, TO_VICT);
  act("$N has been kicked out of $n's group!", FALSE, ch, 0, tch, TO_NOTVICT);

  if (!AFF_FLAGGED(tch, AFF_CHARM))
    stop_follower(tch);
}




ACMD(do_report)
{
  struct char_data *k;
  struct follow_type *f;

  if (!AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char("But you are not a member of any group!\r\n", ch);
    return;
  }
  sprintf(buf, "&W%s &wreports: &w%d&c[&wHp&c], &w%d&c[&wMn&c], &w%d&c[&wMv&c], &w%d%%&c[&wOxi&c]\r\n",
          GET_NAME(ch), GET_HIT(ch), GET_MANA(ch),
          GET_MOVE(ch), GET_OXI(ch));
  CAP(buf);

  k = (ch->master ? ch->master : ch);

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower != ch)
      send_to_char(buf, f->follower);
  if (k != ch)
    send_to_char(buf, k);
  send_to_char("You report to the group.\r\n", ch);
}

ACMD(do_split)
{
  int amount, num, share, rest;
  struct char_data *k;
  struct follow_type *f;

  if (IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if (is_number(buf)) {
    amount = atoi(buf);
    if (amount <= 10) {
      send_to_char("Sorry, you can't do that.\r\n", ch);
      return;
    }
    if (amount > GET_GOLD(ch)) {
      send_to_char("You don't seem to have that much gold to split.\r\n", ch);
      return;
    }
    k = (ch->master ? ch->master : ch);

    if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room))
      num = 1;
    else
      num = 0;

    for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (f->follower->in_room == ch->in_room))
	num++;

    if (num && AFF_FLAGGED(ch, AFF_GROUP)) {
      share = amount / num;
      rest = amount % num;
    } else {
      send_to_char("With whom do you wish to share your gold?\r\n", ch);
      return;
    }

    GET_GOLD(ch) -= share * (num - 1);

    sprintf(buf, "%s splits %d coins; you receive %d.\r\n", GET_NAME(ch),
            amount, share);
    if (rest) {
      sprintf(buf + strlen(buf), "%d coin%s %s not splitable, so %s "
              "keeps the money.\r\n", rest,
              (rest == 1) ? "" : "s",
              (rest == 1) ? "was" : "were",
              GET_NAME(ch));
    }
    if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room)
	&& !(IS_NPC(k)) && k != ch) {
      GET_GOLD(k) += share;
      send_to_char(buf, k);
    }
    for (f = k->followers; f; f = f->next) {
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (f->follower->in_room == ch->in_room) &&
	  f->follower != ch) {
	GET_GOLD(f->follower) += share;
	send_to_char(buf, f->follower);
      }
    }
    sprintf(buf, "You split %d coins among %d members -- %d coins each.\r\n",
	    amount, num, share);
    if (rest) {
      sprintf(buf + strlen(buf), "%d coin%s %s not splitable, so you keep "
                                 "the money.\r\n", rest,
                                 (rest == 1) ? "" : "s",
                                 (rest == 1) ? "was" : "were");
      GET_GOLD(ch) += rest;
    }
    send_to_char(buf, ch);
  } else {
    send_to_char("How many coins do you wish to split with your group?\r\n", ch);
    return;
  }
}

ACMD(do_use)
{
  struct obj_data *mag_item;
 // struct obj_data *object;
  int equipped = 1, i, spellnum, level, remort;
	
	if (IS_NPC(ch))
	return;
	
  level = (GET_OBJ_LEVEL(mag_item)-((LVL_IMMORT - 1)*(GET_OBJ_LEVEL(mag_item)/(LVL_IMMORT - 1))));
    if ((level == 0) && (GET_OBJ_LEVEL(mag_item) > 0)){
     level = (LVL_IMMORT - 1);
     remort = (GET_OBJ_LEVEL(mag_item)/(LVL_IMMORT - 1)) - 1;
    }
    else{
     remort = (GET_OBJ_LEVEL(mag_item)/(LVL_IMMORT - 1));
    }
  
  half_chop(argument, arg, buf);
  if (!*arg) {
    sprintf(buf2, "What do you want to %s?\r\n", CMD_NAME);
    send_to_char(buf2, ch);
    return;
  }
  mag_item = GET_EQ(ch, WEAR_HOLD);

  if (!mag_item || !isname(arg, mag_item->name)) {
    switch (subcmd) {
    case SCMD_RECITE:
    case SCMD_QUAFF:
      equipped = 0;
      if (!(mag_item = get_obj_in_list_vis(ch, arg, ch->carrying))) {
        sprintf(buf2, "You don't seem to have %s %s.\r\n", AN(arg), arg);
        send_to_char(buf2, ch);
        return;
      }
      break;
    case SCMD_STUDY:
    case SCMD_USE:
      sprintf(buf2, "You don't seem to be holding %s %s.\r\n", AN(arg), arg);
      send_to_char(buf2, ch);
      return;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
      return;
    }
  }
  switch (subcmd) {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
      send_to_char("You can only quaff potions.\r\n", ch);
      return;
    }
    if (GET_OBJ_LEVEL(mag_item) > GET_LEVEL(ch)){
    send_to_char("You need more level to quaff that.\r\n", ch);
    return;
        }
   /* if (GET_REMORT(ch) != 0 && GET_REMORT(ch) < remort) {
    	send_to_char("You need more level to quaff that.\r\n", ch);
    return;
}*/
    break;
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
      send_to_char("You can only recite scrolls.\r\n", ch);
      return;
    }
    if (GET_OBJ_LEVEL(mag_item) > GET_LEVEL(ch)){
    send_to_char("You need more level to recite that.\r\n", ch);
    return;
        }
   /* if (GET_REMORT(ch) != 0 && GET_REMORT(ch) < remort) {
    	send_to_char("You need more level to recite that.\r\n", ch);
    return;
}*/
    break;
  case SCMD_STUDY:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SPELLBOOK) {
      send_to_char("You can only study spell books.\r\n", ch);
      return;
    }
    for (i = 1; i <= 3; i++) {
      spellnum = GET_OBJ_VAL(mag_item, i);
      if (spellnum < 0 || spellnum > TOP_SPELL_DEFINE) continue;
      if (spell_info[spellnum].min_level[(int)GET_CLASS(ch)] > GET_LEVEL(ch)) {
        send_to_char("You are not experienced enough.\r\n", ch);
        return;
      }
    }
    break;
  
  case SCMD_USE:
    if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
        (GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
      send_to_char("You can't seem to figure out how to use it.\r\n", ch);
      return;
    }
    break;
  }

  mag_objectmagic(ch, mag_item, buf);
}



ACMD(do_wimpy)
{
  int wimp_lev;

  /* 'wimp_level' is a player_special. -gg 2/25/98 */
  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    if (GET_WIMP_LEV(ch)) {
      sprintf(buf, "Your current wimp level is %s hit points.\r\n",
              add_points(GET_WIMP_LEV(ch)));
      send_to_char(buf, ch);
      return;
    } else {
      send_to_char("At the moment, you're not a wimp.  (sure, sure...)\r\n", ch);
      return;
    }
  }
  if (isdigit(*arg)) {
    if ((wimp_lev = atoi(arg)) != 0) {
      if (wimp_lev < 0)
        send_to_char("Heh, heh, heh.. we are jolly funny today, eh?\r\n", ch);
      else if (wimp_lev > GET_MAX_HIT(ch))
        send_to_char("That doesn't make much sense, now does it?\r\n", ch);
      else if (wimp_lev > (GET_MAX_HIT(ch)/2))
        send_to_char("You can't set your wimp level above half hit points.\r\n", ch);
      else {
        sprintf(buf, "Okay, you'll wimp out if you drop below %s hit points.\r\n",
                add_points(wimp_lev));
        send_to_char(buf, ch);
        GET_WIMP_LEV(ch) = wimp_lev;
      }
    } else {
      send_to_char("Okay, you'll now tough out fights to the bitter end.\r\n", ch);
      GET_WIMP_LEV(ch) = 0;
    }
  } else
    send_to_char("Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n", ch);

  return;

}


#define USAGE "Usage: prompt H M V T\r\n" \
              "  use H for Health, M for Mana Points, T to Mental Points,\r\n" \
	      " V to Movement and E to experience.\r\n" \
              "\r\n" \
              "Example: prompt HV  for [x]Hp [x]Mv >\r\n" \
              "         prompt all for [x]Xp [x]Hp [x]Mn [x]Mp [x]Mv >\r\n" \
              "You can use any combination of HMTVE, ALL to all of them, and NONE for no one.\r\n" \

ACMD(do_display)
{
  size_t i;

  if (IS_NPC(ch)) {
    send_to_char("Mosters don't need displays.  Go away.\r\n", ch);
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(USAGE, ch);
    return;
  }
  if ((!str_cmp(argument, "on")) || (!str_cmp(argument, "all"))) {
    SET_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE | PRF_DISPMENT);
    SET_BIT(PRF2_FLAGS(ch), PRF2_DISPEXP);
  }
  else {
    REMOVE_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE | PRF_DISPMENT);
    REMOVE_BIT(PRF2_FLAGS(ch), PRF2_DISPEXP);

  if ((!str_cmp(argument, "off")) || (!str_cmp(argument, "none"))) {
    send_to_char(OK, ch);
    return;
  }

    for (i = 0; i < strlen(argument); i++) {
      switch (LOWER(argument[i])) {
      case 'h':
        SET_BIT(PRF_FLAGS(ch), PRF_DISPHP);
        break;
      case 'm':
        SET_BIT(PRF_FLAGS(ch), PRF_DISPMANA);
        break;
      case 't':
        SET_BIT(PRF_FLAGS(ch), PRF_DISPMENT);
        break;
      case 'v':
        SET_BIT(PRF_FLAGS(ch), PRF_DISPMOVE);
        break;
      case 'e':
        SET_BIT(PRF2_FLAGS(ch), PRF2_DISPEXP);
        break;
      default:
        send_to_char(USAGE, ch);
        return;
        break;
      }
    }
  }

  send_to_char(OK, ch);
}



ACMD(do_gen_write)
{
  FILE *fl;
  char *tmp, buf[MAX_STRING_LENGTH];
  const char *filename;
  struct stat fbuf;
  time_t ct;

  switch (subcmd) {
  case SCMD_BUG:
    filename = BUG_FILE;
    break;
  case SCMD_TYPO:
    filename = TYPO_FILE;
    break;
  case SCMD_IDEA:
    filename = IDEA_FILE;
    break;
  default:
    return;
  }

  ct = time(0);
  tmp = asctime(localtime(&ct));

  if (IS_NPC(ch)) {
    send_to_char("Monsters can't have ideas - Go away.\r\n", ch);
    return;
  }

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char("That must be a mistake...\r\n", ch);
    return;
  }
  sprintf(buf, "%s %s: %s", GET_NAME(ch), CMD_NAME, argument);
  mudlog(buf, CMP, LVL_IMMORT, FALSE);

  if (stat(filename, &fbuf) < 0) {
    perror("Error statting file");
    return;
  }
  if (fbuf.st_size >= max_filesize) {
    send_to_char("Sorry, the file is full right now.. try again later.\r\n", ch);
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    perror("do_gen_write");
    send_to_char("Could not open the file.  Sorry.\r\n", ch);
    return;
  }
  fprintf(fl, "%-8s (%6.6s) [%5d] %s\n", GET_NAME(ch), (tmp + 4),
          GET_ROOM_VNUM(IN_ROOM(ch)), argument);
  fclose(fl);
  send_to_char("Okay.  Thanks!\r\n", ch);
}



#define TOG_OFF 0
#define TOG_ON  1

#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))
#define PRF2_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF2_FLAGS(ch), (flag))) & (flag))

ACMD(do_gen_tog)
{
  long result;

  const char *tog_messages[][2] = {
    {"You are now safe from summoning by other players.\r\n",
     "You may now be summoned by other players.\r\n"},
    {"Nohassle disabled.\r\n",
     "Nohassle enabled.\r\n"},
    {"Brief mode off.\r\n",
     "Brief mode on.\r\n"},
    {"Compact mode off.\r\n",
     "Compact mode on.\r\n"},
    {"You can now hear tells.\r\n",
     "You are now deaf to tells.\r\n"},
    {"You can now hear auctions.\r\n",
     "You are now deaf to auctions.\r\n"},
    {"You can now hear shouts.\r\n",
     "You are now deaf to shouts.\r\n"},
    {"You can now hear gossip.\r\n",
     "You are now deaf to gossip.\r\n"},
    {"You can now hear the congratulation messages.\r\n",
     "You are now deaf to the congratulation messages.\r\n"},
    {"You can now hear the chat channel.\r\n",
     "You are now deaf to the chat channel.\r\n"},
    {"You can now hear the Wiz-channel.\r\n",
     "You are now deaf to the Wiz-channel.\r\n"},
    {"You are no longer part of the Quest.\r\n",
     "Okay, you are part of the Quest!\r\n"},
    {"You will no longer cross doors without opening them.\r\n",
     "You will now cross doors without opening them.\r\n"},
    {"You will now have your communication repeated.\r\n",
     "You will no longer have your communication repeated.\r\n"},
    {"HolyLight mode off.\r\n",
     "HolyLight mode on.\r\n"},
    {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
     "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
    {"Autoexits disabled.\r\n",
     "Autoexits enabled.\r\n"},
    {"Autoloot off.\r\n",
     "Autoloot on.\r\n"},
    {"Autogold off.\r\n",
     "Autogold on.\r\n"},
    {"Autosplit off.\r\n",
     "Autosplit on.\r\n"},
    {"You are back.\r\n",
     "You are away from keyboard now.\r\n"},
    {"Autoassist off.\r\n",
     "Autoassist on.\r\n"},
    {"WarDome special protocol off.\r\n",
     "WarDome special protocol on.\r\n"},
    {"Mini map off.\r\n",
     "Mini map on.\r\n"},
    {"You can be replied now.\r\n",
     "You can not be replied.\r\n"},
	{"Your recall will point to your house now.\r\n",
	 "Your recall will not point to your house now.\r\n"},
    {"You can now hear de newbie chat.\r\n",
     "You are deaf to newbie chat.\r\n"},
    	{"You can see the newbie TIPS now.\r\n",
     	 "You will no longer see the newbie TIPS,\r\n"},
  {"MSP OFF.\r\n",
     	 "MSP ON.\r\n"},
    {"Paths advice ON.\r\n",
     "Paths advice OFF.\r\n"},
  {"OOC channel ON.\r\n",
     "OOC channel OFF.\r\n"},
  
  };


  if (IS_NPC(ch))
    return;

  switch (subcmd) {
  case SCMD_NOSUMMON:
    result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
    break;
  case SCMD_NOHASSLE:
    result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
    break;
  case SCMD_BRIEF:
    result = PRF_TOG_CHK(ch, PRF_BRIEF);
    break;
  case SCMD_COMPACT:
    result = PRF_TOG_CHK(ch, PRF_COMPACT);
    break;
  case SCMD_NOTELL:
    result = PRF_TOG_CHK(ch, PRF_NOTELL);
    break;
  case SCMD_NOAUCTION:
    result = PRF_TOG_CHK(ch, PRF_NOAUCT);
    break;
  case SCMD_DEAF:
    result = PRF_TOG_CHK(ch, PRF_DEAF);
    break;
  case SCMD_NOGOSSIP:
    result = PRF_TOG_CHK(ch, PRF_NOGOSS);
    break;
  case SCMD_NOGRATZ:
    result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
    break;
  case SCMD_NOWIZ:
    result = PRF_TOG_CHK(ch, PRF_NOWIZ);
    break;
  case SCMD_QUEST:
  	if (PLR_FLAGGED(ch, PLR_SECPLAYER)){
  	send_to_char("Second players can't do part of a quest!\r\n", ch);
    	return;
	}
    result = PRF_TOG_CHK(ch, PRF_QUEST);
    break;
  case SCMD_CROSSDOORS:
    result = PRF_TOG_CHK(ch, PRF_CROSSDOORS);
    break;
  case SCMD_NOREPEAT:
    result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
    break;
  case SCMD_HOLYLIGHT:
    result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
    break;
  case SCMD_SLOWNS:
    result = (nameserver_is_slow = !nameserver_is_slow);
    break;
  case SCMD_AUTOEXIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
    break;
  case SCMD_AUTOLOOT:
    result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
    break;
  case SCMD_AUTOGOLD:
    result = PRF_TOG_CHK(ch, PRF_AUTOGOLD);
    break;
  case SCMD_AUTOSPLIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
    break;
  case SCMD_AFK:
    result = PRF_TOG_CHK(ch, PRF_AFK);
    break;
  case SCMD_AUTOASSIST:
    result = PRF_TOG_CHK(ch, PRF_AUTOASSIST);
    break;
  case SCMD_NOCHAT:
    result = PRF_TOG_CHK(ch, PRF_NOCHAT);
    break;
  case SCMD_WDPROTOCOL:
    result = PRF2_TOG_CHK(ch, PRF2_WDPROTOCOL);
    break;
  case SCMD_MAP:
    result = PRF2_TOG_CHK(ch, PRF2_MAP);
    break;
  case SCMD_NOREPLY:
    result = PRF2_TOG_CHK(ch, PRF2_NOREPEAT);
    break;
  case SCMD_HOUSE_RECALL:
	if (ch->player_specials->saved.house_vnum > 0)
		result = PRF2_TOG_CHK(ch, PRF2_HOUSE_RECALL);
	else {
		send_to_char("You cannot tog this if you don't have a house.\r\n", ch);
		return;
	}
    break;
   case SCMD_NONEWBIE:
	result = PRF2_TOG_CHK(ch, PRF2_NONEWBIE);
    break;
   
    case SCMD_NOTIPS:
     	result = PRF2_TOG_CHK(ch, PRF2_NOTIPS);
  	break;
	
	case SCMD_MSP:
     	result = PLR_TOG_CHK(ch, PLR_MSP);
  	break;
	
	case SCMD_NOPATHS:
	result = PRF2_TOG_CHK(ch, PRF2_NOPATHS);
        break;
        
        case SCMD_NOHOLLER:
	result = PRF2_TOG_CHK(ch, PRF2_NOHOLLER);
    	break;


  default:
    log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
    return;
  }

  if (result)
    send_to_char(tog_messages[subcmd][TOG_ON], ch);
  else
    send_to_char(tog_messages[subcmd][TOG_OFF], ch);

  return;
}

// codigo funcionando perfeito, recoloqueio-o (Archangel)
ACMD(do_sacrifice)
{
   struct obj_data *obj;

   one_argument(argument, arg);

   if (!*arg)
   {
     send_to_char("Sacrifice what?\r\n",ch);
     return;
   }

   if (!(obj = get_obj_in_list_vis(ch, arg, world[ch->in_room].contents)))
   {
     send_to_char("You can't start to sacrifice!\r\n",ch);
     return;
   }

   if (!CAN_WEAR(obj, ITEM_WEAR_TAKE) || !IS_CORPSE(obj))
   {
     send_to_char("You can't sacrifice that!\r\n",ch);
     return;
   }

   if(IS_PC_CORPSE(obj))
   {
   	send_to_char("You can not sacrifice a player corpse.\r\n", ch);
   	return;
   }

   act("$n sacrifices $p.", FALSE, ch, obj, 0, TO_ROOM);
   sprintf(buf,"You sacrifice %s to your god.\r\nYou have been rewarded by your deity.\r\n",(obj)->short_description);
   send_to_char(buf, ch);
   extract_obj(obj);
   GET_GOLD(ch) += 10;
   GET_GOLD_SUM(ch) += 10;
}

ACMD(do_gas)
{
  byte percent;
  bool has_bomb = FALSE;
  struct obj_data *bomb;

  if (!GET_SKILL(ch, SKILL_WHITE_GAS)) {
	  send_to_char("You don't know how.\r\n", ch);
	  return;
  }

  if (GET_MOVE(ch) < 10) {
	  send_to_char("You are too tired.\r\n", ch);
	  return;
  }

  for (bomb = ch->carrying; bomb && !has_bomb; bomb = bomb->next_content) {
	  if (GET_OBJ_VNUM(bomb) == 50) {
		  obj_from_char(bomb);
		  extract_obj(bomb);
		  has_bomb = TRUE;
	  }
  }

  if (!has_bomb) {
	  send_to_char("You need a Gas Bomb to do that.\r\n", ch);
	  return;
  }

  if (AFF_FLAGGED(ch, AFF_GAS))
	  REMOVE_BIT(AFF_FLAGS(ch), AFF_GAS);

  percent = number(1, 101);     /* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_WHITE_GAS)/2 + (10 + con_app[GET_DEX(ch)].hitp)*2) {
	  improve_skill(ch, SKILL_WHITE_GAS);
	  return;
  }

  act("$n attempts to drop gas on the room.", FALSE, ch, 0, 0, TO_ROOM);
  act("$n disappears on the middle of the white gas.", FALSE, ch, 0, 0, TO_ROOM);
  send_to_char("You are hide on the room now.\r\n", ch);
  SET_BIT(AFF_FLAGS(ch), AFF_GAS);
  improve_skill(ch, SKILL_WHITE_GAS);
}

ACMD(do_meditate)
{
  byte percent;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_MEDITATE)){
    send_to_char("You have no idea how.\r\n", ch);
    return;
  }

  send_to_char("You try to start meditate.\r\n", ch);

  percent = number(1, 101);     /* 101% is a complete failure */

  if (GET_POS(ch) != POS_SITTING){
    send_to_char("You can't do this when you aren't sitting.\r\n", ch);
    return;
  }

  if (percent > GET_SKILL(ch, SKILL_MEDITATE)){
    send_to_char("You fail trying to meditate.\r\n", ch);
    return;
  }

  send_to_char("You start to meditate.\r\n", ch);
  SET_BIT(PLR_FLAGS(ch), PLR_MEDITATE);
  improve_skill(ch, SKILL_MEDITATE);
}

#define CAN_LISTEN_BEHIND_DOOR(ch,dir)  \
                (EXIT(ch, dir) && EXIT(ch, dir)->to_room != NOWHERE && \
                 IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED)))


ACMD(do_listen)
{
   struct char_data *tch, *tch_next;
   int percent, found = 0;
   char *heard_nothing = "You don't hear anything unusual.\r\n";
   char *room_spiel    = "$n seems to listen intently for something.";

   percent = number(1,101);

   if(!GET_SKILL(ch, SKILL_LISTEN) || GET_SKILL(ch, SKILL_LISTEN) < percent) {
      send_to_char(heard_nothing, ch);
      return;
   }

      /* no argument means that the character is listening for
       * hidden or invisible beings in the room he/she is in
       */
      for(tch = world[ch->in_room].people; tch; tch = tch_next) {
         tch_next = tch->next_in_room;
         if((tch != ch) && !CAN_SEE(ch, tch) && (GET_LEVEL(tch) < LVL_IMMORT))
            found++;
      }
      if(found) {
	 improve_skill(ch, SKILL_LISTEN);
         if(GET_SKILL(ch, SKILL_LISTEN) >= 70) {
            /* being a higher level is better */
            sprintf(buf, "You hear what might be %d creatures invisible, or hiding.\r\n", MAX(1,(found+number(0,1)-number(0,1))));
         }
         else
            sprintf(buf, "You hear an odd rustling in the immediate area.\r\n");

         send_to_char(buf, ch);
      }
      else
       send_to_char(heard_nothing, ch);
       act(room_spiel, TRUE, ch, 0, 0, TO_ROOM);
       return;

}


/* transformacao em ordem */
#define MAX_TRANS		7

const char *nome_trans[] = {
    "kaiser",
    "sprite",
    "werewolf",
    "sucubus",
    "behemoth",
    "tyrael",
    "slayer"
};

const char *cnome_trans[] = {
    "&gKaiser&n",
    "&ySprite&n",
    "&rWerewolf&n",
    "&mSucubus&n",
    "&bBehemoth&n",
    "&wTyrael&n",
    "&cSlayer&n"
};

const char *ctitle_trans[] = {
    "&Wwith &ggreen&W scales and huge &cwings&n",
    "&ywith big nose and a tiny pair of wings&n",
    "&rwith hair all over the body and a wolf appearance&n",
    "&mwith a very pleasant feminine appareance&n",
    "&bwith demon face and a fat and ugly but powerful body&n",
    "&wwith white-shinning wings and tentacles&n",
    "&cwith a very ugly demon aspect and very powerful muscles&n"
};

#define HP		0
#define MANA		1
#define MOVE		2
#define DAM		3
#define HIT		4
#define AC		5
#define VNUM_EQ		6
#define MENTAL		7
#define MT_GAST		8
#define SKILLT		9
#define MENTMIN		10

int trans_aff[][11] = {
/* qual hp(x)mana(x)move(x)dam(+)hit(+) ac(+)vnumeq mental  mcosttick skill   qtd minima*/
      {  4,    1,     3,    450,  400,  -150, 1266,    175,  -50, SKILL_KAISER,    90},
      {  1,    1,     1,     40,   25,   -20,    0,      5,   -5, SKILL_SPRITE,    10},
      {  2,    1,     1,    100,   70,   -40,    0,     25,  -10, SKILL_WEREWOLF,  20},
      {  2,    1,     1,    140,  100,   -60,    0,     50,  -20, SKILL_SUCUBUS,   30},
      {  2,    1,     2,    220,  170,   -80,    0,    100,  -30, SKILL_BEHEMOTH,  50},
      {  3,    1,     2,    250,  240,  -100,    0,    150,  -40, SKILL_TYRAEL,    60},
      {  4,    1,     3,    400,  300,  -100,    0,    200,  -50, SKILL_SLAYER,    70},
      {  0,    0,     0,      0,    0,     0,    0,      0,    0, 	     0,     0}
};

#define	CHARAC			0
#define ROOM			1

const char *mens_trans[][2] = {
 {
  "&WYou feel a &Cgodly &Rpower &Wchanging your body.&n\r\n&WYou got the hybrid dragon form &gKaiser&W!&n\r\n",
  "&c$n &Wtransforms into a &Rdragon&W!&n"
 },
 {
  "You feel a funny power changing your body.\r\nYou got the form of a tiny but smart sprite!\r\n",
  "$n transforms into a sprite!"
 },
 {
  "You feel the powers of moon changing your body.\r\nYou transform into a werewolf!\r\n",
  "$n transforms into a werewolf!"
 },
 {
  "You feel a magic power changing your body.\r\nYou got the form of the ancient demoness sucubus!\r\n",
  "$n transforms into a sucubus!"
 },
 {
  "You feel a wicked power changing your body.\r\nYou got the form of the fat demon behemoth!\r\n",
  "$n transforms into a behemoth!"
 },
 {
  "You feel a holy power changing your body.\r\nYou got the angel form of the archangel Tyrael, with light tentacles!\r\n",
  "$n transforms into a angel with light tentacles!"
 },
 {
  "You feel a devil power changing your body.\r\nYou got the form of Slayer, the destroyer of humankind!\r\n",
  "$n transforms into the Slayer!"
 }
};

void aumenta_poder(struct char_data * ch)
{
  int door;

  send_to_char("&RYou yell, 'AAAAAAAAAHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH'&n\r\n", ch);
  act("&R$n yells, 'AAAAAAAAAHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH'&n", FALSE, ch, 0, 0, TO_ROOM);

  sprintf(buf, "&R%s yells, 'AAAAAAAAAHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH'&n\r\n", GET_NAME(ch));
  for (door = 0; door < NUM_OF_DIRS; door++)
    if (CAN_GO(ch, door))
      send_to_room(buf, world[ch->in_room].dir_option[door]->to_room);
}

int tem_eq(struct char_data *ch, int trans)
{
	int j = 0;

	if(trans_aff[trans][VNUM_EQ] == 0)
		return FALSE;

    for (j = 0; j < NUM_WEARS; j++)
		if(GET_EQ(ch, j))
			if(GET_OBJ_VNUM(GET_EQ(ch, j)) == trans_aff[trans][VNUM_EQ])
				return TRUE;

	return FALSE;
}

int transformar(struct char_data *ch, int trans)
{
	struct affected_type af[6];
	obj_rnum r_num;
	short int eq = 0;

	if(!(eq = tem_eq(ch, trans)))
		if(!GET_SKILL(ch, trans_aff[trans][SKILLT]))
		   return (FALSE);

	if(GET_MENTAL(ch) < trans_aff[trans][MENTAL])
	   return (FALSE);

	if(GET_MENTAL(ch) < (GET_MAX_MENTAL(ch)*trans_aff[trans][MENTMIN])/100)
	   return (FALSE);

	GET_MENTAL(ch) -= trans_aff[trans][MENTAL];
	GET_TRANS(ch) = trans;

	SET_BIT(AFF2_FLAGS(ch), AFF2_TRANSFORM);

	if (ch->affected)
          while (ch->affected)
            affect_remove(ch, ch->affected);

	af[0].type = trans_aff[trans][SKILLT];
	af[0].duration = -1;
	af[0].modifier = (GET_MAX_HIT(ch)*(trans_aff[trans][HP]-1)+1);
	af[0].location = APPLY_HIT;
	af[0].bitvector = 0;
	af[0].bitvector2 = AFF2_TRANSFORM;
	af[0].bitvector3 = 0;
	affect_to_char(ch, &af[0]);

	af[1].type = trans_aff[trans][SKILLT];
	af[1].duration = -1;
	af[1].modifier = (GET_MAX_MANA(ch)*(trans_aff[trans][MANA]-1)+1);
	af[1].location = APPLY_MANA;
	af[1].bitvector = 0;
	af[1].bitvector2 = AFF2_TRANSFORM;
	af[1].bitvector3 = 0;
	affect_to_char(ch, &af[1]);

	af[2].type = trans_aff[trans][SKILLT];
	af[2].duration = -1;
	af[2].modifier = (GET_MAX_MOVE(ch)*(trans_aff[trans][MOVE]-1)+1);
	af[2].location = APPLY_MOVE;
	af[2].bitvector = 0;
	af[2].bitvector2 = AFF2_TRANSFORM;
	af[2].bitvector3 = 0;
	affect_to_char(ch, &af[2]);

	af[3].type = trans_aff[trans][SKILLT];
	af[3].duration = -1;
	af[3].modifier = trans_aff[trans][DAM]+1;
	af[3].location = APPLY_DAMROLL;
	af[3].bitvector = 0;
	af[3].bitvector2 = AFF2_TRANSFORM;
	af[3].bitvector3 = 0;
	affect_to_char(ch, &af[3]);

	af[4].type = trans_aff[trans][SKILLT];
	af[4].duration = -1;
	af[4].modifier = trans_aff[trans][HIT]+1;
	af[4].location = APPLY_HITROLL;
	af[4].bitvector = 0;
	af[4].bitvector2 = AFF2_TRANSFORM;
	af[4].bitvector3 = 0;
	affect_to_char(ch, &af[4]);

	af[5].type = trans_aff[trans][SKILLT];
	af[5].duration = -1;
	af[5].modifier = trans_aff[trans][AC]-1;
	af[5].location = APPLY_AC;
	af[5].bitvector = 0;
	af[5].bitvector2 = AFF2_TRANSFORM;
	af[5].bitvector3 = 0;
	affect_to_char(ch, &af[5]);

/*	GET_TRANS_HP(ch) = GET_HIT(ch);
	GET_TRANS_MANA(ch) = GET_MANA(ch);
	GET_TRANS_MOVE(ch) = GET_MOVE(ch);*/

	GET_HIT(ch) = GET_MAX_HIT(ch);
	GET_MANA(ch) = GET_MAX_MANA(ch);
	GET_MOVE(ch) = GET_MAX_MOVE(ch);

  	save_char(ch, ch->in_room);
	Crash_crashsave(ch);

	aumenta_poder(ch);
    if(eq)
    {
		r_num = real_object(trans_aff[trans][VNUM_EQ]);
    	sprintf(buf, "&WYou use as your &csource &Wof &Rpower &n%s&W.&n\r\n", obj_proto[r_num].short_description);
    	send_to_char(buf, ch);
	}
	send_to_char(mens_trans[trans][CHARAC], ch);
	act(mens_trans[trans][ROOM], TRUE, ch, 0, 0, TO_NOTVICT);

  return (TRUE);
}

const char *mens2_trans[][2] = {
 {"&WYou feel yourself &Rweak&W. Lost the &gKaiser &Rpower&W!&n\r\n",
  "&c$n &Wcomes back to the normal form&n!"},

 {"You feel yourself bored. Lost the sprite's power!\r\n",
  "$n comes back to the normal form!"},

 {"You feel the moon too far... Lost the werewolf's power!\r\n",
  "$n comes back to the normal form!"},

 {"You feel your sexuality decreasing. Lost the sucubus power!\r\n",
  "$n comes back to the normal form!"},

 {"You feel yourself weak and thin. Lost Behemoth's power!\r\n",
  "$n comes back to the normal form!"},

 {"You lost the light tentacles. Lost Tyrael's power!\r\n",
  "$n comes back to the normal form!"},

 {"You feel yourself useless, worthless and a trash. Lost the Slayer power!\r\n",
  "$n comes back to the normal form!"}
};

int destransformar(struct char_data *ch, int trans)
{
/*	GET_MAX_HIT(ch) /= trans_aff[trans][HP];
	GET_MAX_MANA(ch) /= trans_aff[trans][MANA];
	GET_MAX_MOVE(ch) /= trans_aff[trans][MOVE];
	GET_DAMROLL(ch) =  0;
	GET_HITROLL(ch) =  0;
	GET_AC(ch) =  100;

	GET_HIT(ch) = GET_TRANS_HP(ch);
	GET_MANA(ch) = GET_TRANS_MANA(ch);
	GET_MOVE(ch) = GET_TRANS_MOVE(ch);

	GET_TRANS_HP(ch) = 0;
	GET_TRANS_MANA(ch) = 0;
	GET_TRANS_MOVE(ch) = 0;*/

	REMOVE_BIT(AFF2_FLAGS(ch), AFF2_TRANSFORM);

	if (ch->affected)
          while (ch->affected)
            affect_remove(ch, ch->affected);

	GET_TRANS(ch) = 0;

	save_char(ch, ch->in_room);
	Crash_crashsave(ch);

	send_to_char(mens2_trans[trans][CHARAC], ch);
	act(mens2_trans[trans][ROOM], TRUE, ch, 0, 0, TO_NOTVICT);

  return (TRUE);
}

const char *retorno[] = {
       "&WYou are already &ctransformed&W!&n\r\n",
       "&WYou aren't &ctransformed&W!&n\r\n",
       "&WYou can not &ctransform&C!&n\r\n"
};

ACMD(do_transform)
{
  int men = -1, x = 0, t = 1, y = 0;
  char nome[80];
  
  one_argument(argument, nome);
  
  if(!*nome)
	t = 3;
  else {
       for(; y < MAX_TRANS && t; y++) {
	  if(isname(nome, nome_trans[x])) {
		t = 0;
		if(!AFF2_FLAGGED(ch, AFF2_TRANSFORM)) {
			if(!transformar(ch, x))
			   men = 3;
		} else
			men = 1;
	  } else if(exact_isname(nome, "normal")) {
		t = 0;
		if(AFF2_FLAGGED(ch, AFF2_TRANSFORM)) {
			destransformar(ch, GET_TRANS(ch));
			t = 0;
		} else
			men = 2;
	  } else
		t = 3;
	  x++;
       }
  }

  if(men < 4  && men > 0)
  	send_to_char(retorno[men-1], ch);

  if (t == 3) {
    strcpy(buf2, "&CUse&W: &Btransform &c<");
    for(x = 0; x < MAX_TRANS; x++)
	sprintf(buf2 + strlen(buf2), "&C%s&c/", nome_trans[x]);
    strcat(buf2, "&Wnormal&c>&n\r\n");
    send_to_char(buf2, ch);
  }
}

ACMD(do_blood_ritual)
{
  int percent, cont;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BLOOD_RITUAL)) {
    send_to_char("You have no idea how to do that.\r\n", ch);
    return;
  }

  cont = number(0, GET_LEVEL(ch)*2);

  if(GET_HIT(ch) <= cont)
  {
   send_to_char("You are too weak.\r\n", ch);
   return;
  }

  send_to_char("You called out the darkness.\r\n",ch);

  percent = number(1, 101);	/* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_BLOOD_RITUAL))
  {
   send_to_char("But there were no answers.\r\n", ch);
   return;
  }

  GET_MANA(ch) = MIN(GET_MAX_MANA(ch), GET_MANA(ch) + (cont/4));
  GET_HIT(ch) = GET_HIT(ch) - cont;
  improve_skill(ch, SKILL_BLOOD_RITUAL);
  send_to_char("Darkness answered your call. Your blood turns into power.\r\n", ch);
}

ACMD(do_recharge)
{
	
	struct obj_data *wand;
	int level;
        long price ;

	one_argument(argument, arg);

	if (IS_NPC(ch))
		return;

	if (GET_ROOM_VNUM(ch->in_room) != 1127) {
		send_to_char("You cannot do that here.\r\n", ch);
		return;
	}

	if (!*arg) {
		send_to_char("&gMaltzabor tells you, 'Recharge what?'&n\r\n", ch);
		return;
	}

	if (!(wand = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		send_to_char("&gMaltzabor tells you, 'Recharge what?!'&n\r\n", ch);
		return;
	}

	if ((GET_OBJ_TYPE(wand) != ITEM_WAND) && (GET_OBJ_TYPE(wand) != ITEM_STAFF)) {
		send_to_char("&gMaltzabor tells you, 'This item doesn't seem to be a &cWand&g or &cStaff&g.'&n\r\n", ch);
		return;
	}

	level = (GET_OBJ_LEVEL(wand)-((LVL_IMMORT - 1)*(GET_OBJ_LEVEL(wand)/(LVL_IMMORT - 1))));
    if ((level == 0) && (GET_OBJ_LEVEL(wand) > 0))
		level = (LVL_IMMORT - 1);

// 	price = (level * 900); desequilibrado demais
//        price  = 5000000;       
       if (IS_OBJ_STAT(wand, ITEM_QUEST)) {
        	send_to_char("&gMaltzabor tells you, 'You can't recharge quest items!&n'\r\n", ch);
        	return;
        }
        
       if ((GET_OBJ_LEVEL(wand)) < 50) {
       	price = 50000;
}
else   
       price = (GET_OBJ_LEVEL(wand)) * 700;
		
         

	if (GET_GOLD(ch) < price) {
		send_to_char("&gMaltzabor tells you, 'You don't have money!'&n\r\n", ch);
		return;
	}

	if (wand->obj_flags.value[1] == wand->obj_flags.value[2]) {
		send_to_char("&gMaltzabor tells you, 'This object is already recharged!'&n\r\n", ch);
		return;
	}

	GET_GOLD(ch) -= price;
	wand->obj_flags.value[2]++;
	send_to_char("&gMaltzabor tells you, 'One charge restored.'&n\r\n", ch);
}
#define MAX_WMOTD_LENGTH        255
ACMD(do_message)
{
  extern struct message_data mess_info;
  extern void write_message_to_file();

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (IS_NPC(ch))
    send_to_char("The message of the day is fine... go away.\r\n", ch);
  else if ((PLR_FLAGGED(ch, PLR_NOTITLE)) || (PLR_FLAGGED(ch, PLR_NOSHOUT)))
    send_to_char("You can't post a message -- you shouldn't have abused it!\r\n", ch);
   else if (GET_LEVEL(ch) < LVL_IMMORT)/* Feel free to change the level.*/
    send_to_char("You can't change the message yet. Sit here and wait a while.\r\n",ch);
  else if (!str_cmp(argument, "remove")) {
    mess_info.writer[0] = '\0';     
    mess_info.message[0] = '\0';
    mess_info.time = 0;
    write_message_to_file();
    send_to_char("Message removed.\r\n",ch);
  }else {
    SET_BIT(PLR_FLAGS(ch), PLR_MESSAGING | PLR_WRITING);
    send_to_char("Write your message. End with '/s' on a new line.\r\n"
                 "If you want to remove the message just type: message remove.\r\n", ch);
    act("$n begins to jot down a message.", TRUE, ch, 0, 0, TO_ROOM);
    ch->desc->str = (char **) malloc(sizeof(char *));
    *(ch->desc->str) = NULL;
    ch->desc->max_str = MAX_WMOTD_LENGTH;
  }
}
ACMD(do_craft)
{

	  struct obj_data *obj;
	  int acerto, prob;
	 
	 half_chop(argument, arg, buf);
	  
	  
	 
	if (!*arg){
	send_to_char("Fix what?\r\n", ch);
	return;
	}

	if (!GET_SKILL(ch, SKILL_CRAFT)){
    send_to_char("You have no idea how.\r\n", ch);
    return;
  }
        if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
    return;
  }	
  	if (GET_OBJ_COND(obj) > 99)
  	{
  		send_to_char("Your item is new already!\r\n", ch);
  		return;
  		}
  	if (GET_OBJ_COND(obj) <= 2)
  	{ 
  		send_to_char("&COnly a miracle could repair that piece of junk!&n\r\n", ch);
  		return;
  	}
  
   	if (((GET_OBJ_TYPE(obj)) == ITEM_WAND) || ((GET_OBJ_TYPE(obj)) == ITEM_STAFF) ||
        ((GET_OBJ_TYPE(obj)) == ITEM_TREASURE) || ((GET_OBJ_TYPE(obj)) == ITEM_POTION) ||
        ((GET_OBJ_TYPE(obj)) == ITEM_OTHER) || ((GET_OBJ_TYPE(obj)) == ITEM_TRASH) ||
        ((GET_OBJ_TYPE(obj)) == ITEM_CONTAINER) || ((GET_OBJ_TYPE(obj)) == ITEM_NOTE) ||
        ((GET_OBJ_TYPE(obj)) == ITEM_DRINKCON) || ((GET_OBJ_TYPE(obj)) == ITEM_KEY) ||
        ((GET_OBJ_TYPE(obj)) == ITEM_FOOD) || ((GET_OBJ_TYPE(obj)) == ITEM_PEN) ||
        ((GET_OBJ_TYPE(obj)) == ITEM_BOAT) || ((GET_OBJ_TYPE(obj)) == ITEM_SCROLL)){

     
   	send_to_char("You can just fix armors and weapons.\r\n", ch);
   	return;
}
  	
  	
	acerto = number(0, 101);
	prob = GET_SKILL(ch, SKILL_CRAFT);
	
	if (acerto > prob){
	GET_OBJ_COND(obj) = 1;
	send_to_char("&rHow clumsy you can really be? You ruined your equipment!&n\r\n", ch);
	return;
	
}
	 
        else {
	improve_skill(ch, SKILL_CRAFT);
	GET_OBJ_COND(obj) = 95;
	send_to_char("&YGood job! Your equipment seems to be new!&n\r\n", ch);
	return;
}
	}
	
ACMD(do_changeicq)
{
  int icq;

  /* 'wimp_level' is a player_special. -gg 2/25/98 */
  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    if (GET_ICQ(ch)) {
      sprintf(buf, "Your current icq number is %d .\r\n",
              (GET_ICQ(ch)));
      send_to_char(buf, ch);
      return;
    } else {
      send_to_char("You don't have an icq number\r\n", ch);
      return;
    }
  }
  if (isdigit(*arg)) {
    if ((icq = atoi(arg)) != 0) {
      if (icq < 0)
        send_to_char("Heh, heh, heh.. we are jolly funny today, eh?\r\n", ch);
      
      else {
        sprintf(buf, "Okay, your icq number is now %d .\r\n",
                (icq));
        send_to_char(buf, ch);
        GET_ICQ(ch) = icq;
      }
    } else {
      send_to_char("Okay, your icq number is now empty.\r\n", ch);
      GET_ICQ(ch) = 0;
    }
  } else
    send_to_char("Specify your icq number.  (0 to disable)\r\n", ch);

  return;

}
ACMD(do_namechange){
  struct char_data *vict;  char *oldname;
  int i,j;
  char arg1[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  char tmp_name[MAX_STRING_LENGTH];

  half_chop(argument, arg1, argument);
  half_chop(argument, arg2, argument);
 
  if ((!*arg1) || (!*arg2)) {
    send_to_char("Usage: namechange <character> <new name>\r\n", ch);
    return;
  }
 
  if (!(vict = get_player_vis(ch, arg1, 0))) {
    send_to_char("They aren't here.\r\n", ch);
    return;
  }
 

  if (find_name(arg2)>-1)  {
    send_to_char("There's already somebody with this name.\r\n",ch);
    return;
  }
 

  if ((_parse_name(arg2, tmp_name))||strlen(tmp_name) < 2 ||
       strlen(tmp_name) > MAX_NAME_LENGTH || !Valid_Name(tmp_name) ||
       fill_word(tmp_name) || reserved_word(tmp_name)) {
    send_to_char("Illegal name, retry.\r\n", ch);
    return;
  }
 
  for (i = 0; i <= top_of_p_table; i++)
    if (player_table[i].id == GET_IDNUM(vict))
      break;
  
  if (player_table[i].id != GET_IDNUM(vict)){
    send_to_char("BUG: error in player table!!!!",ch);
    log("BUG: error in player table for %s with idnum %d",
      GET_NAME(ch),GET_IDNUM(ch));
    return;
  }
  
  oldname = strdup(GET_NAME(vict));
 
  //player_table[i].name = strdup(arg2);
  for (j = 0; (*(player_table[i].name + j) = LOWER(*(arg2 + j))); j++);
  
  strcpy(vict->player.name, CAP(arg2));  
 
  sprintf(buf, "(GC) %s has changed the name of %s to %s.", GET_NAME(ch),GET_NAME(vict), arg2);
  mudlog(buf, BRF, LVL_IMMORT, TRUE);
 
  save_char(vict, vict->in_room);
 
  send_to_char("Ok.\r\n",ch);
  send_to_char("Your name has changed.\r\n",vict);
}
 
