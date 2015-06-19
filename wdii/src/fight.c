/* ************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "buffer.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "dg_scripts.h"
#include "quest.h"
#include "clan.h"

/* Structures */
struct char_data *combat_list = NULL;   /* head of l-list of fighting chars */
struct char_data *next_combat_list = NULL;
struct spell_info_type spell_info[TOP_SPELL_DEFINE + 1];

/* External structures */
extern struct con_app_type con_app[];
extern struct index_data *obj_index;
extern struct index_data *mob_index;
extern struct str_app_type str_app[];
extern struct dex_app_type dex_app[];
extern struct room_data *world;
extern struct message_list fight_messages[MAX_MESSAGES];
extern struct obj_data *object_list;
extern struct aq_data *aquest_table;
extern int pk_allowed;          /* see config.c */
extern int auto_save;           /* see config.c -- not used in this file */
extern int free_rent;           /* see config.c */
extern int max_exp_gain;        /* see config.c */
extern int max_exp_loss;        /* see config.c */
extern int top_of_world;
extern int max_npc_corpse_time, max_pc_corpse_time;
extern int glad_bet_amnt;
extern int pit_mob_vnum;
extern sh_int r_mortal_start_room;
extern const int wear_order_index[NUM_WEARS];

/* External procedures */
char *fread_action(FILE * fl, int nr);
ACMD(do_flee);
ACMD(do_get);
ACMD(do_split);
int backstab_mult(int level);
int thaco(int ch_class, int level);
int ok_damage_shopkeeper(struct char_data * ch, struct char_data * victim);
int level_exp(int remort, int level);
void Crash_rentsave(struct char_data * ch, int cost);
char *make_bar(int val, int max, int len);
void diag_char_to_char(struct char_data * i, struct char_data * ch);
void DamageStuff(struct char_data *ch);
int alvo_better(struct char_data *ch);
int contar_briga(struct char_data *ch);
struct char_data *melhor_alvo(struct char_data *ch);
void mob_ia(struct char_data *ch);
int mag_savingthrow(struct char_data *ch , int type) ;
void brag(struct char_data * ch, struct char_data * victim);//mob dor
void play_sound(struct char_data *ch, char *sound, int type);

/* local functions */
void perform_group_gain(struct char_data * ch, int base, struct char_data * victim);
void dam_message(int dam, struct char_data * ch, struct char_data * victim, int w_type);
void appear(struct char_data * ch);
void load_messages(void);
void check_killer(struct char_data * ch, struct char_data * vict);
void make_corpse(struct char_data * ch);
void change_alignment(struct char_data * ch, struct char_data * victim);
void death_cry(struct char_data * ch);
void raw_kill(struct char_data * ch, struct char_data * killer);
void die(struct char_data * ch, struct char_data * killer);
void die_in_dt(struct char_data * ch);
void pkill(struct char_data *ch, struct char_data * victim);
void group_gain(struct char_data * ch, struct char_data * victim);
void solo_gain(struct char_data * ch, struct char_data * victim);
char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
void perform_violence(void);
//int return_wanted_gain(struct char_data * ch, struct char_data * victim);//comentar aki
int can_pk(struct char_data * ch, struct char_data * vt);
int app_style[][3];
void dragon_spec(struct char_data *ch , int message) ; // procedimento generico para dragoes


void dragon_spec(struct char_data *ch, int message)
{
 int dam, num  ;
 struct char_data *vict ;


 vict = FIGHTING(ch) ;

 dam = GET_LEVEL(ch)  ;
 dam = dam + (GET_REMORT(ch) * 150) ;

 num = 1 + (GET_REMORT(ch)/4)  ;

 while(num > 0)
 {
  vict = FIGHTING(ch) ;

  if((vict == NULL) || (ch->in_room != vict->in_room))
   return ;

  if(mag_savingthrow(vict, SAVING_BREATH))
   dam = dam / 2 ;

  num-- ;

  damage(ch, vict, dam, message) ;
 }
}

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
{
  {"hit", "hits"},              /* 0 */
  {"sting", "stings"},
  {"whip", "whips"},
  {"slash", "slashes"},
  {"bite", "bites"},
  {"bludgeon", "bludgeons"},    /* 5 */
  {"crush", "crushes"},
  {"pound", "pounds"},
  {"claw", "claws"},
  {"maul", "mauls"},
  {"thrash", "thrashes"},       /* 10 */
  {"pierce", "pierces"},
  {"blast", "blasts"},
  {"punch", "punches"},
  {"stab", "stabs"}
};

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))

/* The Fight related routines */


int encumberance_level(struct char_data *ch)
{
  int weight_worn=0,i, level=0, items_worn = 0;

  for (i=0; i < NUM_WEARS; i++)
    if (ch->equipment[i]){
      weight_worn += ch->equipment[i]->obj_flags.weight;
      items_worn++;
    }
  //weight_worn /= 2;
  //items_worn /= 3;
  if ((IS_CARRYING_W(ch) + weight_worn) > (4*CAN_CARRY_W(ch))/3)
    level = 8;
  else if ((IS_CARRYING_W(ch) + weight_worn) > (3.5*CAN_CARRY_W(ch))/3)
    level = 7;
  else if ((IS_CARRYING_W(ch) + weight_worn) > (3*CAN_CARRY_W(ch))/3)
    level = 6;
  else if ((IS_CARRYING_W(ch) + weight_worn) > (2.5*CAN_CARRY_W(ch))/3)
    level = 5;
  else if ((IS_CARRYING_W(ch) + weight_worn) > (2*CAN_CARRY_W(ch))/3)
    level = 4;
  else if ((IS_CARRYING_W(ch) + weight_worn) > (1.5*CAN_CARRY_W(ch))/3)
    level = 3;
  else if ((IS_CARRYING_W(ch) + weight_worn) > (CAN_CARRY_W(ch))/3)
    level = 2;
  if ((IS_CARRYING_N(ch) + items_worn) > (4*CAN_CARRY_N(ch))/5)
    level += 8;
  else if ((IS_CARRYING_N(ch) + items_worn) > (3.5*CAN_CARRY_N(ch))/5)
    level += 7;
  else if ((IS_CARRYING_N(ch) + items_worn) > (3*CAN_CARRY_N(ch))/5)
    level += 6;
  else if ((IS_CARRYING_N(ch) + items_worn) > (2.5*CAN_CARRY_N(ch))/5)
    level += 5;
  else if ((IS_CARRYING_N(ch) + items_worn) > (2*CAN_CARRY_N(ch))/5)
    level += 4;
  else if ((IS_CARRYING_N(ch) + items_worn) > (1.5*CAN_CARRY_N(ch))/5)
    level += 3;
  else if ((IS_CARRYING_N(ch) + items_worn) > (CAN_CARRY_N(ch))/5)
    level += 2;

  level *= 6;
  return(level);
}


void appear(struct char_data * ch)
{
  if (affected_by_spell(ch, SPELL_INVISIBLE))
    affect_from_char(ch, SPELL_INVISIBLE);

  REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE | AFF_HIDE);

  if (GET_LEVEL(ch) < LVL_IMMORT)
    act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
  else
    act("You feel a strange presence as $n appears, seemingly from nowhere.",
        FALSE, ch, 0, 0, TO_ROOM);
}

void load_messages(void)
{
  FILE *fl;
  int i, type;
  struct message_type *messages;
  char chk[128];

  if (!(fl = fopen(MESS_FILE, "r"))) {
    sprintf(buf2, "SYSERR: Error reading combat message file %s", MESS_FILE);
    perror(buf2);
    exit(1);
  }
  for (i = 0; i < MAX_MESSAGES; i++) {
    fight_messages[i].a_type = 0;
    fight_messages[i].number_of_attacks = 0;
    fight_messages[i].msg = 0;
  }


  fgets(chk, 128, fl);
  while (!feof(fl) && (*chk == '\n' || *chk == '*'))
    fgets(chk, 128, fl);

  while (*chk == 'M') {
    fgets(chk, 128, fl);
    sscanf(chk, " %d\n", &type);
    for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
         (fight_messages[i].a_type); i++);
    if (i >= MAX_MESSAGES) {
      log("SYSERR: Too many combat messages.  Increase MAX_MESSAGES and recompile.");
      exit(1);
    }
    CREATE(messages, struct message_type, 1);
    fight_messages[i].number_of_attacks++;
    fight_messages[i].a_type = type;
    messages->next = fight_messages[i].msg;
    fight_messages[i].msg = messages;

    messages->die_msg.attacker_msg = fread_action(fl, i);
    messages->die_msg.victim_msg = fread_action(fl, i);
    messages->die_msg.room_msg = fread_action(fl, i);
    messages->miss_msg.attacker_msg = fread_action(fl, i);
    messages->miss_msg.victim_msg = fread_action(fl, i);
    messages->miss_msg.room_msg = fread_action(fl, i);
    messages->hit_msg.attacker_msg = fread_action(fl, i);
    messages->hit_msg.victim_msg = fread_action(fl, i);
    messages->hit_msg.room_msg = fread_action(fl, i);
    messages->god_msg.attacker_msg = fread_action(fl, i);
    messages->god_msg.victim_msg = fread_action(fl, i);
    messages->god_msg.room_msg = fread_action(fl, i);
    fgets(chk, 128, fl);
    while (!feof(fl) && (*chk == '\n' || *chk == '*'))
      fgets(chk, 128, fl);
  }

  fclose(fl);
}


void update_pos(struct char_data * victim)
{
  if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
    return;
  else if (GET_HIT(victim) > 0 && AFF_FLAGGED(victim, AFF_SLEEP))
	affect_from_char(victim, SPELL_SLEEP);
  else if (GET_HIT(victim) > 0)
    GET_POS(victim) = POS_STANDING;
  else if (GET_HIT(victim) <= -11)
    GET_POS(victim) = POS_DEAD;
  else if (GET_HIT(victim) <= -6)
    GET_POS(victim) = POS_MORTALLYW;
  else if (GET_HIT(victim) <= -3)
    GET_POS(victim) = POS_INCAP;
  else
    GET_POS(victim) = POS_STUNNED;
}


void check_killer(struct char_data * ch, struct char_data * vict)
{
  if (ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
     return;

    if (!PLR_FLAGGED(vict, PLR_KILLER) && !PLR_FLAGGED(vict, PLR_THIEF) && !PLR_FLAGGED(ch, PLR_KILLER) && !IS_NPC(ch) && !IS_NPC(vict) && (ch != vict))
  {
      char buf[256];
      SET_BIT(PLR_FLAGS(ch), PLR_KILLER);
      sprintf(buf, "PC Killer bit set on %s for initiating attack on %s at %s.", GET_NAME(ch), GET_NAME(vict), world[vict->in_room].name);
      mudlog(buf, BRF, LVL_IMMORT, TRUE);
      send_to_char("If you want to be a PLAYER KILLER, so be it...\r\n", ch);
  }
}


/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data * ch, struct char_data * vict)
{
  if (ch == vict)
    return;

  if (FIGHTING(ch)) {
    core_dump();
    return;
  }

  ch->next_fighting = combat_list;
  combat_list = ch;

  if (AFF_FLAGGED(ch, AFF_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);
  
  FIGHTING(ch) = vict;
  GET_POS(ch) = POS_FIGHTING;

/*  if (!pk_allowed)
    check_killer(ch, vict);*/
}

void unentangle(struct char_data *ch)
{
  if (AFF_FLAGGED(ch, AFF_TANGLED))
    {
      send_to_char("You are finally free of the vines that bind.\r\n", ch);
      affect_from_char(ch, SPELL_ENTANGLE);
      act("$n is free of the vines that bind.",TRUE,ch,0,0,TO_ROOM);
    }
}

/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data * ch)
{
  struct char_data *temp;

  if (ch == next_combat_list)
    next_combat_list = ch->next_fighting;

  REMOVE_FROM_LIST(ch, combat_list, next_fighting);
  ch->next_fighting = NULL;
  FIGHTING(ch) = NULL;
  GET_POS(ch) = POS_STANDING;
  BASHTIME(ch) = 0;

  if (IS_AFFECTED(ch, AFF_BERZERK)) {
	if (!IS_NPC(ch))
		send_to_char("&nYou become humble again.&n\n\r", ch);
	act("&n$n becomes humble again.&n", TRUE, ch, 0, 0, TO_ROOM);
	REMOVE_BIT(AFF_FLAGS(ch), AFF_BERZERK);
  }

  update_pos(ch);
  unentangle(ch);

}

void make_corpse(struct char_data * ch)
{
  struct obj_data *corpse, *o;
  struct obj_data *money;
  int i;

  /* check if trying to re-nuke a corpse, bail otherwise
     since a corpse was prob already made, -taerin 4/22/99 */
  if (ch == NULL || ch->in_room == NOWHERE)
    return;

    if (ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
  {

    corpse = create_obj();

    corpse->item_number = NOTHING;
    corpse->in_room = NOWHERE;
    corpse->name = str_dup("head");

    sprintf(buf2, "%s's head is lying here.", GET_NAME(ch));
    corpse->description = str_dup(buf2);
    sprintf(buf2, "%s's head", GET_NAME(ch));
    corpse->short_description = str_dup(buf2);

    GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
    GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
    GET_OBJ_EXTRA(corpse) = ITEM_NODONATE;
    GET_OBJ_VAL(corpse, 0) = 0;   /* You can't store stuff in a corpse */
    GET_OBJ_VAL(corpse, 3) = (0);   /* no corpse identifier */
    GET_OBJ_WEIGHT(corpse) = 0;
    GET_OBJ_RENT(corpse) = 100000;
	
 } else {
    corpse = create_obj();

    corpse->item_number = NOTHING;
    corpse->in_room = NOWHERE;
    corpse->name = str_dup("corpse");

    sprintf(buf2, "The corpse of %s is lying here.", GET_NAME(ch));
    corpse->description = str_dup(buf2);

    sprintf(buf2, "the corpse of %s", GET_NAME(ch));
    corpse->short_description = str_dup(buf2);

    GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
    if (IS_NPC(ch))
		GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
    else 
		GET_OBJ_WEAR(corpse) = !ITEM_WEAR_TAKE; 
    GET_OBJ_EXTRA(corpse) = ITEM_NODONATE;
    GET_OBJ_VAL(corpse, 0) = 0;   // You can't store stuff in a corpse
    GET_OBJ_VAL(corpse, 3) = (IS_NPC(ch) ? 2 : 1);   // corpse identifier
    GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
    GET_OBJ_RENT(corpse) = 100000;
	
	/*if (!IS_NPC(ch))
		GET_OBJ_VAL(corpse, 2) = GET_IDNUM(ch); eu comentei aqui ipslore*/

    if (IS_NPC(ch))
      GET_OBJ_TIMER(corpse) = max_npc_corpse_time;
    else
      GET_OBJ_TIMER(corpse) = max_pc_corpse_time;

	/*if (IS_SET(PLR_FLAGS(ch), PLR_NOPK)) 
		GET_OBJ_LEVEL(corpse) = 0;
	else
		GET_OBJ_LEVEL(corpse) = GET_LEVEL(ch);
	GET_OBJ_VAL(corpse, 1) = GET_REMORT(ch); aqui tambem, espero nao ter feito merda*/

     corpse->contains = ch->carrying;  // transfer character's inventory to the corpse
     	for (o = corpse->contains; o != NULL; o = o->next_content)
             o->in_obj = corpse;
             object_list_new_owner(corpse, NULL);


        for (i = 0; i < NUM_WEARS; i++) // transfer character's equipment to the corpse
             if (GET_EQ(ch, i))
             obj_to_obj(unequip_char(ch, i), corpse);


        if (GET_GOLD(ch) > 0) // transfer gold
      {
        if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc)) // following 'if' clause added to fix gold duplication loophole
       {
            money = create_money(GET_GOLD(ch));
            obj_to_obj(money, corpse);
       }
       GET_GOLD(ch) = 0;
     }
       ch->carrying = NULL;
       IS_CARRYING_N(ch) = 0;
       IS_CARRYING_W(ch) = 0;
    }
    obj_to_room(corpse, ch->in_room);
}


/* When ch kills victim */
void change_alignment(struct char_data * ch, struct char_data * victim)
{
  return  ;

  /*
   * new alignment change algorithm: if you kill a monster with alignment A,
   * you move 1/16th of the way to having alignment -A.  Simple and fast.
   */
  GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / 16;
}



void death_cry(struct char_data * ch)
{
  int door, was_in;

  act("&rYour blood freezes as you hear &R$n's &rdeath cry.&n", FALSE, ch, 0, 0, TO_ROOM);
  was_in = ch->in_room;

  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (CAN_GO(ch, door)) {
      ch->in_room = world[was_in].dir_option[door]->to_room;
      act("&rYour blood freezes as you hear someone's death cry.&n", FALSE, ch, 0, 0, TO_ROOM);
      ch->in_room = was_in;
    }
  }
}

void raw_kill(struct char_data * ch, struct char_data * killer)

{
  if (FIGHTING(ch))
    stop_fighting(ch);
	
  if (!ROOM_FLAGGED(ch->in_room, ROOM_ARENA)) {
	while (ch->affected)
		affect_remove(ch, ch->affected);
	if (killer) {
		autoquest_trigger_check(killer, ch, NULL, AQ_MOB_KILL);
		if (death_mtrigger(ch, killer))
			death_cry(ch);
	} else
		death_cry(ch);
	
	if (ch && killer)
		if ((!IS_NPC(ch)) && (!IS_NPC(killer)))
			REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER | PLR_THIEF);

	if (killer && !IS_NPC(killer)) {
		GET_DIED(ch) += 1;
		GET_KILLED(ch) = 0;
	}
	GET_DIED_S(ch) += 1;
     
	
  	if(GET_MOB_VNUM(ch) != 11 && GET_MOB_VNUM(ch) != 15 &&
     	GET_MOB_VNUM(ch) != 16 && GET_MOB_VNUM(ch) != 20)
	
	make_corpse(ch);

	if (!IS_NPC(ch))
	{
			play_sound(ch, "CHARDTH7.wav", SND_ROOM);
	  		sprintf(buf, "\r\n&W%s, after the last event you lost your fisical body...&n\r\n"
  	       		"&WFor now your are dead.&n\r\n"
  	       		"&WWait in the purgatory for your judgement.&n\r\n\r\n", GET_NAME(ch));
	  		send_to_char(buf, ch);
  	  		GET_HIT(ch) = 1;
  	  		GET_MANA(ch) = 1;
  	  		GET_MOVE(ch) = 1;
	  		
	  		if(GET_LEVEL(ch) < LVL_IMMORT)
	    		SET_BIT(PLR_FLAGS(ch), PLR_DEAD);

	    		if (ch->in_room != NOWHERE)
	      		char_from_room(ch);
	    		char_to_room(ch, real_room(10));
	    		act("&B$n &bappears in a strange wortex of light, created by the 	&BW&ba&Br&bD&Bo&bm&Be.&n", FALSE, ch, 0, 0, TO_NOTVICT);
          		look_at_room(ch, 0);
	  		update_pos(ch);

	} else {
	  		if (free_rent)
	     		Crash_rentsave(ch, 0);
	   		extract_char(ch);           /* Char is saved in extract char */
	}
  } else {
	   if(!IS_NPC(ch)){
	    act("$n is magically whisked away.", FALSE, ch, 0, 0, TO_ROOM);
	    make_corpse(ch);
	    char_from_room(ch);
	    char_to_room(ch, r_mortal_start_room);
	    look_at_room(ch, 0);
	    act("&w$n falls from the sky.", FALSE, ch, 0, 0, TO_ROOM);
	    act("&w$n looks like he had a bad day", FALSE, ch, 0, 0, TO_ROOM);
	    GET_HIT(ch) = ch->desc->hp;
	    GET_MANA(ch) = ch->desc->mana;
	    GET_MOVE(ch) = ch->desc->move;
	    GET_MENTAL(ch) = GET_MAX_MENTAL(ch);
	    GET_ARENA_DIED(ch) += 1;
	    GET_OXI(ch) = GET_MAX_OXI(ch);
	   if (ch->affected)
	     while (ch->affected)
	       affect_remove(ch, ch->affected);
	     GET_POS(ch) = POS_STANDING;
	   } else {
	     act("$n's body glows brightly and then slowly fades away.\r\n"
        	, FALSE, ch, 0, 0, TO_ROOM);
	        extract_char(ch);
           }
  }
  if (killer)
    autoquest_trigger_check(killer, NULL, NULL, AQ_MOB_SAVE);
}

void die_in_dt(struct char_data * ch)
{

  if (FIGHTING(ch))
    stop_fighting(ch);

  while (ch->affected)
    affect_remove(ch, ch->affected);

  death_cry(ch);
play_sound(ch, "CHARDTH4.wav", SND_CHAR);
  
  //GET_DIED_S(ch) += 1;

	  sprintf(buf, "\r\n&W%s, after the last action you lost your fisical body...&n"
	  	       "&WFor now your are dead.&n\r\n"
	  	       "&WWait in the purgatory for your judgement.&n\r\n\r\n", GET_NAME(ch));
	  send_to_char(buf, ch);
  	  GET_HIT(ch) = 1;
  	  GET_MANA(ch) = 1;
  	  GET_MOVE(ch) = 1;
	  SET_BIT(PLR_FLAGS(ch), PLR_DEAD);
	    if (ch->in_room != NOWHERE)
	      char_from_room(ch);
	    char_to_room(ch, 3);
	    act("&B$n &bappears in a strange wortex of light, created by the &BW&ba&Br&bD&Bo&bm&Be.&n", FALSE, ch, 0, 0, TO_NOTVICT);
	  update_pos(ch);
          look_at_room(ch, 0);
}

void pkill(struct char_data * ch, struct char_data * victim)
{
if (FIGHTING(ch))
    stop_fighting(ch);

  while (ch->affected)
    affect_remove(ch, ch->affected);

  death_cry(ch);

  make_corpse(ch);
 
  GET_DIED_S(ch) += 1;
  
	  sprintf(buf, "\r\n&W%s, after the last action you lost your fisical body...&n"
	  	       "&WFor now your are dead.&n\r\n"
	  	       "&WWait in the purgatory for your judgement.&n\r\n\r\n", GET_NAME(ch));
	  send_to_char(buf, ch);
  	  GET_HIT(ch) = 1;
  	  GET_MANA(ch) = 1;
  	  GET_MOVE(ch) = 1;
	  LIBERDADE(ch) = 30;
	  
	  int somaganha=0, pontosganha=0, somaperde=0, pontosperde=0, bonus=0;
		somaganha = ((GET_LEVEL(ch)*GET_KILLED(ch))); 
		pontosganha = (GET_REMORT(ch)*GET_KILLED(ch)/10);
		somaperde = ((GET_LEVEL(ch)*GET_KILLED(ch))); 
		pontosperde = (GET_REMORT(ch)*GET_KILLED(ch)/10);
		bonus = GET_KILLED(ch)*2;
		
		GET_GOLD(ch) -= somaperde;
		GET_GOLD(victim) += somaganha;
		GET_POINTS(ch) -= pontosperde;
		GET_POINTS(victim) += pontosganha;
	  	GET_QP(victim) += bonus;
	  	sprintf(buf, "&cBonus: &Y+&w%d &yquest points.&n\r\n", bonus);
	   	send_to_char(buf, victim);
	  SET_BIT(PLR_FLAGS(ch), PLR_DEAD);
	  if (ch->in_room != NOWHERE)
	      char_from_room(ch);
	    char_to_room(ch, 3);
	    act("&B$n &bappears in a strange wortex of light, created by the &BW&ba&Br&bD&Bo&bm&Be.&n", FALSE, ch, 0, 0, TO_NOTVICT);
	  update_pos(ch);
          look_at_room(ch, 0);
	  GET_KILLED(ch) = 0;
	  REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER | PLR_THIEF);
}	

void die(struct char_data * ch, struct char_data * killer)
{
  
  if (ch && killer)
    if ((!IS_NPC(ch)) && (!IS_NPC(killer)))
      REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER | PLR_THIEF);

  raw_kill(ch, killer);
}


void perform_group_gain(struct char_data * ch, int base,
                             struct char_data * victim)
{
  int share, exp_max, penalti = 0;

  if(IS_NPC(ch))
   return;

  if(!IS_NPC(victim))
   return;

  if (ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
   return;

  if(GET_LEVEL(ch) < LVL_IMMORT)
	exp_max = (level_exp(GET_REMORT(ch), GET_LEVEL(ch) + 1) / MAX(1,(15 - (GET_REMORT(ch)))));
  else
	exp_max = 1;

  share = MIN(exp_max, MAX(1, base));

  /* Calculate level-difference bonus */
  if (IS_NPC(victim)) {
    if(GET_LEVEL(ch) > GET_LEVEL(victim)) {
	penalti = GET_LEVEL(ch)-GET_LEVEL(victim);
	penalti /= 10;
	penalti = (penalti > 10 ? 10 : penalti);
    }
    if(penalti > 0) {
	share *= (penalti == 10 ? 11 - penalti : 10 - penalti);
	share /= 12;
    }
  }

  if(AFF_FLAGGED(victim, AFF_HOLDED))
	share -= share / 10;

  if(GET_REMORT(ch) > (GET_REMORT(victim)+2))
  	share /= (GET_REMORT(ch) - GET_REMORT(victim));

  if(GET_REMORT(ch) == 0) // incentivo de jogo para
    share = share * 1.4 ; // players nao remorters

  if (share > 1) {
    sprintf(buf2, "&wYou receive your share of experience -- %s points.&n\r\n", add_points(share));
    send_to_char(buf2, ch);
  } else
    send_to_char("&wYou receive your share of experience -- &Wone &wmeasly little point!&n\r\n", ch);

  gain_exp(ch, share);
  change_alignment(ch, victim);
}


void group_gain(struct char_data * ch, struct char_data * victim)
{
  int tot_members, base, tot_gain;
  struct char_data *k;
  struct follow_type *f;

  if (!(k = ch->master))
    k = ch;

  if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room))
    tot_members = 1;
  else
    tot_members = 0;

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower->in_room == ch->in_room)
      tot_members++;

  /* round up to the next highest tot_members */
  tot_gain = (GET_EXP(victim)) + tot_members - 1;

  /* prevent illegal xp creation when killing players */
  if (!IS_NPC(victim))
    tot_gain = MIN(max_exp_loss * 2 / 3, tot_gain);

  if (tot_members >= 1)
    base = MAX(1, tot_gain / tot_members);
  else
    base = 0;

  if (AFF_FLAGGED(k, AFF_GROUP) && k->in_room == ch->in_room)
    perform_group_gain(k, base, victim);

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower->in_room == ch->in_room)
      perform_group_gain(f->follower, base, victim);
}


void solo_gain(struct char_data * ch, struct char_data * victim)
{
  int exp, exp_max, penalti = 0;

  if(IS_NPC(ch))
   return;

  if(!IS_NPC(victim))
   return;

  if (ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
   return;

  if(GET_LEVEL(ch) < LVL_IMMORT)
	exp_max = (level_exp(GET_REMORT(ch), GET_LEVEL(ch) + 1) / MAX(1,(15 - (GET_REMORT(ch)))));
  else
	exp_max = 1;

  exp = MIN(exp_max, GET_EXP(victim));

  /* Calculate level-difference bonus */
  if (IS_NPC(victim)) {
    if(GET_LEVEL(ch) > GET_LEVEL(victim)) {
	penalti = GET_LEVEL(ch)-GET_LEVEL(victim);
	penalti /= 10;
	penalti = (penalti > 10 ? 10 : penalti);
    }
    if(penalti > 0) {
	exp *= (penalti == 10 ? 11 - penalti : 10 - penalti);
	exp /= 12;
    }
  }

  if(AFF_FLAGGED(victim, AFF_HOLDED))
	exp -= exp / 10;

  exp = MAX(exp, 1);

  if(GET_REMORT(ch) > GET_REMORT(victim))
  	exp /= (GET_REMORT(ch) - GET_REMORT(victim));

  if(GET_REMORT(ch) == 0) // incentivo de jogo para
    exp = exp * 1.4 ; // players nao remorters

  if (exp > 1) {
    sprintf(buf2, "&wYou receive &W%s &wexperience points.&n\r\n", add_points(exp));
    send_to_char(buf2, ch);
  } else
    send_to_char("&wYou receive &Wone &wlousy experience point.&n\r\n", ch);

  gain_exp(ch, exp);
  change_alignment(ch, victim);
}


char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural)
{
  static char buf[256];
  char *cp = buf;

  for (; *str; str++) {
    if (*str == '#') {
      switch (*(++str)) {
      case 'W':
        for (; *weapon_plural; *(cp++) = *(weapon_plural++));
        break;
      case 'w':
        for (; *weapon_singular; *(cp++) = *(weapon_singular++));
        break;
      default:
        *(cp++) = '#';
        break;
      }
    } else
      *(cp++) = *str;

    *cp = 0;
  }                             /* For */

  return (buf);
}
const char *porradinha[] =
{
"&ntickles",
"&nbruises",
"&nscratches",
"&ngrazes",
"&nnicks",
};

const char *porrada[] =
{
"&nscars",
"&nhits",
"&ninjures",
"&nwounds",
"&nmauls",
"&nmaims",
"&nmangles",
"&nmars",
"&GLACERATES",
"&BDECIMATES",
};

const char *porradona[] =
{
"&MDEVASTATES",
"&bERADICATES",
"&WOBLITERATES",
"&gEXTIRPATES",
"&CINCINERATES",
"&CMUTILATES",
"&RDISMEMBERS",
"&RMASSACRES",
"&BDISMEMBERS",
"&BRENDS",
"&Y- BLASTS -",
"&M-= DEMOLISHES =-",
"&Y** &WSHREDS &Y**",
"&M****&W DESTROYS&M ****",
};

const char *super_porrada[] =
{
"&W*****&C PULVERIZES&W *****",
"&B-=- VAPORIZES -=-",
"&M<-==-> &CATOMIZES &M<-==->",
"&C<&W-:-&C> &WASPHYXIATES &C<&W-:-&C>",
"&W<-*->&C RAVAGES &W<-*->",
"&M<>*<> &CFISSURES &M<>*<>",
"&Y<*>&R<*> &bLIQUIDATES &R<*>&Y<*>",
"&c<*>&Y<*> &GEVAPORATES &Y<*>&c<*>",
"&Y<-=-> &RSUNDERS &Y<-=->",
"&W<&G-&W><&G=&W> &GTEARS INTO &W<&G=&W><&G-&W>",
"&Y<->&B*&Y<=> &bWASTES &Y<->&B*&Y<=>",
"&R<-&W+&R-> &WCREMATES &R<-&W+&R->",
"&M<*>&R<*> ANNIHILATES <*>&M<*>",
"&g<--&G*&g-->&b IMPLODES &g<--&G*&g-->",
"&C<&W-&C><-=-> &WEXTERMINATES &C<-=->&C<&W-&C>",
};

const char *mega_porrada[] =
{
"&R<-=->&G<-=-> &RSHATTERS&G <-=->&R<-=->",
"&W<&G-&W:&G-&W> &YSLAUGHTERS &W<&G-&W:&G-&W>",
"&M<->&C<-*-> &MRUPTURES &C<-*->&M<->",
"&Y<-*->&R<*> &GNUKES &R<*>&Y<-*->",
"&B*&w#&B* &YEXPLODES &B*&w#&B*",
"&W<[=-=]&b<&w:&b> &WGLACIATES &b<&w:&b>&W[=-=]>",
"&R<-&w:&R-&w*&R-&w:&R-> &RMETEORITES &R<-&w:&R-&w*&R-&w:&R->",
"&Y<-&R:&Y-&R*&Y-&R:&Y-> &WSUPERNOVAS &Y<-&R:&Y-&R*&Y-&R:&Y->",
};

void nome_maiusculo(char *saida, char *entrada);

void nome_minusculo(char *saida, char *entrada)
{
	strcpy(saida, entrada);
	*saida = LOWER(entrada[0]);
}

void dam_message(int dam, struct char_data * ch, struct char_data * victim, int w_type)
{
	char pancada[5000], mens[MAX_INPUT_LENGTH], nome[2000];

	if(dam == 0)
		strcpy(pancada, "misses");
	else if(dam <= 10)
		strcpy(pancada, porradinha[(dam/2)-1]);
	else if(dam <= 30)
		strcpy(pancada, porrada[(dam/3)-1]);
	else if(dam <= 100)
		strcpy(pancada, porradona[(dam/7)-1]);
	else if(dam <= 300)
		strcpy(pancada, super_porrada[(dam/20)-1]);
	else if(dam <= 1000)
		strcpy(pancada, mega_porrada[(dam/125)-1]);
	else
		strcpy(pancada, mega_porrada[7]);

	w_type -= TYPE_HIT;

	sprintf(mens, "&c$U$n &W%s %s &c$u$N&W.&n",	attack_hit_text[w_type].singular,
			pancada);
  	act(mens, FALSE, ch, NULL, victim, TO_NOTVICT);

	nome_minusculo(nome, GET_NAME(victim));
	sprintf(mens, "&cYour &W%s %s &R%s&W. [&R%d&W]&n\r\n",
			attack_hit_text[w_type].singular, pancada, nome, dam);
	send_to_char(mens, ch);

	nome_maiusculo(nome, GET_NAME(ch));
	sprintf(mens, "&R%s &W%s %s &cyou&W. [&R%s%d&W]&n\r\n", nome,
			attack_hit_text[w_type].singular, pancada,
			((dam == 0) ? "" : "-"), dam);
	send_to_char(mens, victim);
}


/*
 * message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
int skill_message(int dam, struct char_data * ch, struct char_data * vict,
                      int attacktype)
{
  int i, j, nr;
  struct message_type *msg;

  struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);

  for (i = 0; i < MAX_MESSAGES; i++) {
    if (fight_messages[i].a_type == attacktype) {
      nr = dice(1, fight_messages[i].number_of_attacks);
      for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
        msg = msg->next;

      if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IMMORT)) {
        act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
        act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
        act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      } else if (dam != 0) {
        if (GET_POS(vict) == POS_DEAD) {
          send_to_char(CCYEL(ch, C_CMP), ch);
          act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
          send_to_char(CCNRM(ch, C_CMP), ch);

          send_to_char(CCRED(vict, C_CMP), vict);
          act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
          send_to_char(CCNRM(vict, C_CMP), vict);

          act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
        } else {
          send_to_char(CCYEL(ch, C_CMP), ch);
          act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
          send_to_char(CCNRM(ch, C_CMP), ch);

          send_to_char(CCRED(vict, C_CMP), vict);
          act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
          send_to_char(CCNRM(vict, C_CMP), vict);

          act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
        }
      } else if (ch != vict) {  /* Dam == 0 */
        send_to_char(CCYEL(ch, C_CMP), ch);
        act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
        send_to_char(CCNRM(ch, C_CMP), ch);

        send_to_char(CCRED(vict, C_CMP), vict);
        act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
        send_to_char(CCNRM(vict, C_CMP), vict);

        act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      }
      return 1;
    }
  }
  return 0;
}

/*
 * Alert: As of bpl14, this function returns the following codes:
 *      < 0     Victim died.
 *      = 0     No damage.
 *      > 0     How much damage done.
 */
int damage(struct char_data * ch, struct char_data * victim, int dam, int attacktype)
{
  long local_gold = 0;
  int gb, ga;
  char local_buf[256];
  int clan_num;
  int dam_to_mana = 0, lucky, exp_max, x = 0, mvgasto, oxgasto = 0;
  struct obj_data *objX = GET_EQ(ch, WEAR_WIELD);
 // struct obj_data * obj;

  if (GET_POS(victim) <= POS_DEAD) {
    log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
                GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
    die(victim, ch);
    return 0;                   /* -je, 7/7/92 */
  }

  if(attacktype < 699 && attacktype > 614)
  {
  	sprintf(buf, "SYSERR: %s tentou derrubar usando arma alterada! Avisar Zaaroth!", GET_NAME(ch));
	mudlog(buf, BRF, LVL_ELDER, TRUE);
	return 0;
  }

  if(AFF2_FLAGGED(ch, AFF2_TERROR)) {
   send_to_char("You are too panicky to fight!!!\r\n", ch);
   return (0);
  }

  if (GET_MOB_VNUM(victim) == 16) {
    send_to_char("You hit the air when you cross the illusion.\r\n", ch);
    return 0;
  }

  /* peaceful rooms */
  if ((ch != victim && ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) || ((ch != victim) && (ROOM_AFFECTED(ch->in_room, RAFF_FSANCTUARY)))) {
    send_to_char("&wThis room just has such a peaceful, easy feeling...&n\r\n", ch);
    return 0;
  }


  /* travar mobs atacando players fora do pk range */
  if(IS_NPC(ch) && ch->master) {
    if(!IS_NPC(victim) && !IS_NPC(ch->master) && !(attacktype == TYPE_SUFFERING || attacktype == SPELL_POISON)){
      if(!can_pk(ch->master, victim)){
        return 0;
      }
    }
  }

  /* travar players atacando mobs charmados fora do pk range */
  if(IS_NPC(victim) && victim->master) {
    if(!IS_NPC(ch) && !IS_NPC(victim->master) && !(attacktype == TYPE_SUFFERING || attacktype == SPELL_POISON)) {
      if(!can_pk(ch, victim->master)) {
        send_to_char("You cannot kill a charmed mob out of PK RANGE.\r\n", ch);
        return 0;
      }
    }
  }


  if(!IS_NPC(victim) && !IS_NPC(ch) && !(attacktype == TYPE_SUFFERING || attacktype == SPELL_POISON)){
    if(!can_pk(ch, victim)){
      send_to_char("&WYou can not &Rkill &Wyour opponent right now.&n\r\n", ch);
      return 0;
    }
  }

  /* entangled */
  if (IS_AFFECTED(ch, AFF_TANGLED)) {
    send_to_char("You struggle against your bonds...\r\n", ch);

    if(number(0, 30) < number(10, 50))
       unentangle(ch);

    return 0;
  }

  /* shopkeeper protection */
  if (!ok_damage_shopkeeper(ch, victim))
    dam = 0;

  /* You can't damage an immortal! */
  if (!IS_NPC(victim) && (GET_LEVEL(victim) >= LVL_IMMORT))
    dam = 0;

  if (victim != ch) {
    /* Start the attacker fighting the victim */
    if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
      set_fighting(ch, victim);

    /* Start the victim fighting the attacker */
    if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL)) {
      set_fighting(victim, ch);
      if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
        remember(victim, ch);
    }
  }

  if(GET_RACE(victim) == RACE_ORC)
	if(objX)
    		if (GET_OBJ_VNUM(objX) == 10113)
     			dam = dam * 2;



  if (attacktype < 0 || attacktype >= MAX_SPELLS) { // Aplicavel soh para ataques com arma.. -Ips
//DANOS ELEMENTAIS
//-----------MOBS FIRE------------
	if (MOB_FLAGGED(victim, MOB_FIREMOB))
		if (GET_EQ(ch, WEAR_WIELD) && IS_OBJ_STAT((GET_EQ(ch, WEAR_WIELD)), ITEM_WATER))
			dam = dam * 2;
	
	if (MOB_FLAGGED(victim, MOB_FIREMOB))
		if (GET_EQ(ch, WEAR_WIELD) && !IS_OBJ_STAT((GET_EQ(ch, WEAR_WIELD)), ITEM_WATER))
			dam = dam * 0.7;
//------------MOBS WATER----------	
	if (MOB_FLAGGED(victim, MOB_WATERMOB))
		if (GET_EQ(ch, WEAR_WIELD) && IS_OBJ_STAT((GET_EQ(ch, WEAR_WIELD)), ITEM_FIRE))
			dam = dam * 2;
	
	if (MOB_FLAGGED(victim, MOB_WATERMOB))
		if (GET_EQ(ch, WEAR_WIELD) && !IS_OBJ_STAT((GET_EQ(ch, WEAR_WIELD)), ITEM_FIRE))
			dam = dam * 0.7;

	if ( FOREST(ch->in_room) && !IS_NPC(ch) ) { // -ips
		if ( GET_SKILL(ch, SKILL_FOREST_WILDERNESS) > number(1, 99) ) {
			dam = dam * 1.2;
			improve_skill(ch, SKILL_FOREST_WILDERNESS);
		}
	}

  }

  if (attacktype < 0 || attacktype >= MAX_SPELLS) {
    /* gastar movimento quando bate :) */
    if(!IS_NPC(ch) && GET_LEVEL(ch) < LVL_IMMORT && GET_LEVEL(ch) > 5) {
      if (GET_MOVE(ch) <= 5) {
         send_to_char("&WYou can't hit, you are too exhausted to move yourself.&n\r\n", ch);
         return (0);
      } else {
         mvgasto = number(1, 3);
         GET_MOVE(ch) -= mvgasto;
      }
    }

     oxgasto = 1 ;
  // gastar oxigenação
    if(!IS_NPC(ch) && GET_LEVEL(ch) < LVL_IMMORT && GET_LEVEL(ch) > 5){
     if (GET_OXI(ch) <= 5){
        send_to_char("&CYou can't hit anymore. &WYou are out of gas, take a break!&n\r\n", ch);
         return (0);
     } else
        GET_OXI(ch) -= oxgasto;
    }
  }

  /* If you attack a pet, it hates your guts */
  if (victim->master == ch)
    stop_follower(victim);

  /* If the attacker is invisible, he becomes visible */
  if (AFF_FLAGGED(ch, AFF_INVISIBLE | AFF_HIDE))
    appear(ch);

  /* Cut damage in half if victim has sanct, to a minimum 1 */
  if (AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >= 2)
    dam -= dam / 6;

 if(AFF2_FLAGGED(victim, AFF2_DEATHDANCE))
      dam *= 1.3;

 if(AFF2_FLAGGED(ch, AFF2_DEATHDANCE))
      dam *= 1.3;

 if (GET_STYLE(ch) == 4) // style savage -Ips & Str
 	dam *= 1.2;

 if (GET_STYLE(ch) == 5) // style aiming -Ips & Str
 	dam *= 0.9;

 if(IS_EVIL(ch) && (AFF_FLAGGED(victim, AFF_PROTECT_EVIL)))
	 dam -= dam / 8;

 if(IS_GOOD(ch) && (AFF_FLAGGED(victim, AFF_PROTECT_GOOD)))
	 dam -= dam / 8;

 if(AFF2_FLAGGED(ch, AFF2_MUIR1))
 {  
  if(IS_EVIL(victim))
    dam = dam + GET_LEVEL(ch);

  if(IS_NEUTRAL(victim))
    dam = dam + GET_LEVEL(ch)/2;
 }

  if(AFF2_FLAGGED(ch, AFF2_TEMPUS))
   dam = dam + GET_LEVEL(ch)/3 ; 

 
  if(AFF2_FLAGGED(victim, AFF2_ILMANATEUR1))
   dam = dam * 0.5 ; 

  if(AFF2_FLAGGED(victim, AFF2_BANSHEE) && !AFF2_FLAGGED(ch, AFF2_BANSHEE)){
   if( dam/2 > 0 && number(0,1))
   {
    damage(victim,ch, dam/5, SPELL_BANSHEE_AURA);
    if(ch == NULL)
     return 0 ;
    if (GET_POS(ch) <= POS_DEAD)
     return 0;
   }
  }

  /* Cut damage in half if victim has satan, to a minimum 1 */
  if (AFF_FLAGGED(victim, AFF_SATAN) && dam >= 2)
    dam -= dam / 5;

  /* Cut damage in half if victim has god, to a minimum 1 */
  if (AFF_FLAGGED(victim, AFF_GOD) && dam >= 2)
    dam -= dam / 5;

  if (!IS_NPC(victim) && GET_SKILL(victim, SKILL_AGILITY)
   && number(0, 360) <= GET_SKILL(victim, SKILL_AGILITY) / 2 + GET_DEX(victim) / 15
   && !(attacktype == TYPE_SUFFERING || attacktype == SPELL_POISON)){
    improve_skill(victim, SKILL_AGILITY);
    dam /= 2;
    x = 1;
  }

  /* Check for PK if this is not a PK MUD */
/*  if (!pk_allowed) {
    check_killer(ch, victim);
    if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != victim))
      dam = 0;
  }*/

  /* Set the maximum damage per round and subtract the hit points */
  lucky = number(0, 150);
  if (GET_LUK(ch) > lucky) {
      if ((attacktype >= TYPE_HIT) && (attacktype <= TYPE_STAB))
	 dam = MAX(MIN((dam * 1.25), 5000), 0);
      else
         dam = MAX(MIN((dam * 1.25), 10000), 0);

  } else if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_CRITICAL_ATTACK) > number(1, 150)) {
	  
	  if ((attacktype >= TYPE_HIT) && (attacktype <= TYPE_STAB))
		  dam = MAX(MIN((dam * 1.175), 5000), 0);
	  else
		  dam = MAX(MIN((dam * 1.175), 10000), 0);
	improve_skill(ch, SKILL_CRITICAL_ATTACK);
  } else
       if ((attacktype >= TYPE_HIT) && (attacktype <= TYPE_STAB))
	 dam = MAX(MIN(dam, 5000), 0);
       else
         dam = MAX(MIN(dam, 10000), 0) ;

   if (!IS_NPC(victim) && (GET_LEVEL(victim) <= 5 ) && (GET_LEVEL(ch) <= 6 )){
       dam = dam/1.5;
   }
   //teste do fire elemental
   
   
   
 /*  if (IS_NPC(victim) && AFF2_FLAGGED(victim, AFF2_FIREMOB)){
    dam = 0;
    act("Your enemy seems to resist!.", FALSE, ch, 0, victim, TO_CHAR);
}

    if (IS_NPC(victim) && (objX, GET_OBJ_TYPE(objX) == ITEM_WEAPON && (IS_OBJ_STAT(objX, ITEM_FIRE)))){
    	dam -= dam * 1.5;
    	act("You burn $N with your elemental fire sword.", FALSE, ch, 0, victim, TO_CHAR);
}
*/

  /* FIRESHIELD retorna um ataque com 1/3 do damage de quem atacou */
  if (dam >= 0) {
   if ((attacktype >= TYPE_HIT) && (attacktype <= TYPE_STAB) &&
     (number(1, 200) < GET_LEVEL(victim)) )   {
      if (IS_AFFECTED(victim, AFF_FIRESHIELD) &&
         !IS_AFFECTED(ch, AFF_FIRESHIELD))      {
	if (damage(victim, ch, MAX(1,MIN((dam/(number(1,4))), 100)),
           TYPE_UNDEFINED) ){
          if(ch == NULL)
           return 0 ;
          if (GET_POS(ch) <= POS_DEAD)
           return 0;
          act("You burned in the $N's fire shield.", FALSE, ch, 0, victim, TO_CHAR);
          act("$U$n burns in your fire shield.", FALSE, ch, 0, victim, TO_VICT);
	}
      }
    }
  }

  if (affected_by_spell(victim, SPELL_MANA_SHIELD)) {
    dam_to_mana = dam * 1.4 ;
    if (dam_to_mana > GET_MANA(victim)) {
      affect_from_char(victim, SPELL_MANA_SHIELD);
      act("&w$U$n destroys $N's magic barrier!!&n", FALSE, ch, NULL, victim, TO_NOTVICT);
      act("&wYou destroy $N's magic barrier!!&n", FALSE, ch, NULL, victim, TO_CHAR);
      act("&w$U$n destroys your magic barrier!!&n", FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
      dam_to_mana = GET_MANA(victim);
    }
    GET_MANA(victim) -= dam_to_mana;
    dam = dam - (dam_to_mana/1.7) ;
  }

  GET_HIT(victim) -= dam;

  /* Gain exp for the hit */
  if(GET_LEVEL(ch) < LVL_IMMORT)
	exp_max = (level_exp(GET_REMORT(ch), GET_LEVEL(ch) + 1) / 200);
  else
	exp_max = 1;

  if (ch != victim && IS_NPC(victim) && !ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
    gain_exp(ch, MIN(7500, (((GET_EXP(victim)*0.1)*dam)/(GET_LEVEL(ch)*100))));

  update_pos(victim);

  /*
   * skill_message sends a message from the messages file in lib/misc.
   * dam_message just sends a generic "You hit $n extremely hard.".
   * skill_message is preferable to dam_message because it is more
   * descriptive.
   *
   * If we are _not_ attacking with a weapon (i.e. a spell), always use
   * skill_message. If we are attacking with a weapon: If this is a miss or a
   * death blow, send a skill_message if one exists; if not, default to a
   * dam_message. Otherwise, always send a dam_message.
   */
  if(!x){
    if (!IS_WEAPON(attacktype)){
    	sprintf(buf, "&W[&Y%d&W]&w-&c> ", dam);
	send_to_char(buf, ch);
	skill_message(dam, ch, victim, attacktype);
    }else {
      if (GET_POS(victim) == POS_DEAD || dam == 0) {
        if (!skill_message(dam, ch, victim, attacktype))
          dam_message(dam, ch, victim, attacktype);
      } else {
        dam_message(dam, ch, victim, attacktype);
      }
    }
  } else {
    act("&yYou &Yevade&y $N &yattack and suffer a lower &Ydamage&y!&n", FALSE, victim, 0, ch, TO_CHAR);
    act("&y$U$n &Yevades&y your &yattack and suffer a lower &Ydamage&y!&n", FALSE, victim, 0, ch, TO_VICT);
    act("&y$U$n &Yevades&y $N's &yattack and suffer a lower &Ydamage&y!&n", FALSE, victim, 0, ch, TO_NOTVICT);
  }

  if(!IS_NPC(victim) && !(attacktype == TYPE_SUFFERING || attacktype == SPELL_POISON))
     DamageStuff(victim);

  /* Use send_to_char -- act() doesn't send message if you are DEAD. */
  switch (GET_POS(victim)) {
  case POS_MORTALLYW:
    act("&r$U$n is mortally wounded, and will die soon, if not aided.&n", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("&RYou are mortally wounded, and will die soon, if not aided.&n\r\n", victim);
    break;
  case POS_INCAP:
    act("&r$U$n is incapacitated and will slowly die, if not aided.&n", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("&RYou are incapacitated an will slowly die, if not aided.&n\r\n", victim);
    break;
  case POS_STUNNED:
    act("&r$U$n is stunned, but will probably regain consciousness again.&n", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("&RYou're stunned, you just can see the stars around you.&n\r\n", victim);
    break;
  case POS_DEAD:
    act("&R$U$n is dead!  &RR&r.&RI&r.&RP.&n", FALSE, victim, 0, 0, TO_ROOM);
    send_to_char("&RYou are dead!  Sorry...&n\r\n", victim);
    break;

  default:                      /* >= POSITION SLEEPING */
    if (dam > (GET_MAX_HIT(victim) / 4))
      act("&rThat really did &RHURT!&n", FALSE, victim, 0, 0, TO_CHAR);

    if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 6)) {
      sprintf(buf2, "&rYou wish that your wounds would stop &RBLEEDING &rso much!&n\r\n");
      send_to_char(buf2, victim);
      if (ch != victim && MOB_FLAGGED(victim, MOB_WIMPY))
        do_flee(victim, NULL, 0, 0);
    }
  }


  /* stop someone from fighting if they're stunned or worse */
  if ((GET_POS(victim) <= POS_STUNNED) && (FIGHTING(victim) != NULL))
    stop_fighting(victim);

  /* Uh oh.  Victim died. */
  if (GET_POS(victim) == POS_DEAD) {
    if ((ch != victim) && (IS_NPC(victim) || victim->desc)) {
      if (AFF_FLAGGED(ch, AFF_GROUP))
        group_gain(ch, victim);
      else
        solo_gain(ch, victim);
    }
   // Clan War System. Taerom. 07/2003
    if (!IS_NPC(victim))
    {
     if (!IS_NPC(ch))
     {	  
      if(clans[posicao_clan(GET_CLAN(ch))].em_guerra == GET_CLAN(victim))
      {
       if (GET_CLAN(ch) != GET_CLAN(victim))
       {
		clan_num = posicao_clan(GET_CLAN(ch));
		clans[clan_num].pkvit += 1;
		clans[clan_num].grana += (1000 * GET_CLAN_POS(victim));
		salvar_clan(GET_CLAN(ch));
		clan_num = posicao_clan(GET_CLAN(victim));
		clans[clan_num].pkder += 1;
		clans[clan_num].grana -= (1000 * GET_CLAN_POS(victim));
		salvar_clan(GET_CLAN(victim));
         }
 	}
       }
      } 
   // fim Clan War
   
    if (!IS_NPC(victim)) {
       
      if(!IS_NPC(ch))
      	GET_KPS(ch) += 1;

      if (ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
          GET_ARENA_KILLED(ch) += 1;
      else
          GET_KILLED(ch) += 1;

      sprintf(buf2, "%s (%d) %s by %s (%d) at %s [%d]",
              GET_NAME(victim), GET_LEVEL(victim),
              (IS_NPC(ch)?"killed":"assassinated"),
              GET_NAME(ch), GET_LEVEL(ch),
              world[victim->in_room].name, GET_ROOM_VNUM(victim->in_room));
      mudlog(buf2, BRF, LVL_ELDER, TRUE);
      if (IS_NPC(ch))
      brag(ch, victim);//avacalhador
    	
    } else {
      if(!IS_NPC(ch))
       	GET_KMS(ch) += 1;
    }
	
      	
      if (MOB_FLAGGED(ch, MOB_MEMORY))
        forget(ch, victim);

    if (IS_NPC(victim)) {
      local_gold = GET_GOLD(victim);
      sprintf(local_buf, "%ld", (long)local_gold);//victim eh o cara q morreu
    }
 
    if (!IS_NPC(ch) && !IS_NPC(victim) && !ROOM_FLAGGED(victim->in_room, ROOM_ARENA))//se o assassino nao for mob manda o morto pra dt
		{
		pkill(victim, ch);//morto
		
	}
		
	else
   die(victim, ch);
   //return__gain(ch, victim);//comentar aki

    gb = 0;
    ga = 0;

    if (IS_NPC(victim) && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOLOOT)) {
      send_to_char("\r\n", ch);
      gb = GET_GOLD(ch);
//      do_get(ch,"all corpse",0,0);
      ga = GET_GOLD(ch);
    }
    if (IS_NPC(victim) && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOGOLD) &&
       (local_gold > 0) && !PRF_FLAGGED(ch, PRF_AUTOLOOT)) {
      send_to_char("\r\n", ch);
      gb = GET_GOLD(ch);
//      do_get(ch,"all.gold corpse",0,0);
      ga = GET_GOLD(ch);
    }
    if (!IS_NPC(ch) && IS_AFFECTED(ch, AFF_GROUP) && (local_gold > 0) &&
        PRF_FLAGGED(ch, PRF_AUTOSPLIT) && (PRF_FLAGGED(ch, PRF_AUTOLOOT)
        || PRF_FLAGGED(ch, PRF_AUTOGOLD)))
      if (ga > gb)
        do_split(ch,local_buf,0,0);


    return -1;
  }
  return dam;
}

int prof_ac(struct char_data * ch)
{
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
  int i, acprof = 0;

  for (i = MAX_SPELLS + 1; i < MAX_SKILLS + 1; i++)
   if (!IS_NPC(ch))
    if ((i >= SKILL_HIT && i <= SKILL_STAB) && GET_SKILL(ch, i) > 0)
      if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON && (GET_OBJ_VAL(wielded, 3) == (i - SKILL_HIT))){
 	acprof = GET_SKILL(ch, i)/2;
        improve_skill(ch, i);
      }

 return (acprof);
}

int defender(struct char_data *ch, struct char_data *victim)
{
   double x;

   x = app_style[GET_STYLE(victim)+1][0];
   x *= 3;
   x /= 10;
   x = 1 - x;

   if (!IS_NPC(victim) && (GET_SKILL(victim, SKILL_PARRY) > 0) && 
(number(1, 450) < ((GET_SKILL(victim, SKILL_PARRY)*x)-encumberance_level(victim))) && 
(GET_POS(victim) == POS_FIGHTING)) {
    act("&BYou skillfully &Cparry&B $N attack!&n", FALSE, victim, 0, ch, TO_CHAR);
    act("&B$n skillfully &Cparries&B your attack!&n", FALSE, victim, 0, ch, TO_VICT);
    act("&B$n skillfully &Cparries&B $N attack!&n", FALSE, victim, 0, ch, TO_NOTVICT);
    improve_skill(victim, SKILL_PARRY);
    return 1;
  }
  else if (!IS_NPC(victim) && (GET_SKILL(victim, SKILL_DODGE) > 0) && 
(number(1, 450) < ((GET_SKILL(victim, SKILL_DODGE)*x)-encumberance_level(victim))) && 
(GET_POS(victim) == POS_FIGHTING)) {
    act("&BYou skillfully &Cdodge&B $N attack!&n", FALSE, victim, 0, ch, TO_CHAR);
    act("&B$n skillfully &Cdodges&B your attack!&n", FALSE, victim, 0, ch, TO_VICT);
    act("&B$n skillfully &Cdodges&B $N attack!&n", FALSE, victim, 0, ch, TO_NOTVICT);
    improve_skill(victim, SKILL_DODGE);
    return 1;
  }

  else if (!IS_NPC(victim) && (GET_SKILL(victim, SKILL_TUMBLE) > 0) && 
(number(1, 450) < ((GET_SKILL(victim, SKILL_TUMBLE)*x)-encumberance_level(victim))) && 
(GET_POS(victim) == POS_FIGHTING)) {
    act("&BYou skillfully &Ctumble&B $N attack!&n", FALSE, victim, 0, ch, TO_CHAR);
    act("&B$n skillfully &Ctumbles&B your attack!&n", FALSE, victim, 0, ch, TO_VICT);
    act("&B$n skillfully &Ctumbles&B $N attack!&n", FALSE, victim, 0, ch, TO_NOTVICT);
    improve_skill(victim, SKILL_TUMBLE);
    return 1;
  }
  else if (!IS_NPC(victim) && (GET_SKILL(victim, SKILL_SHIELD_BLOCK) > 
0) && (GET_EQ(victim, WEAR_SHIELD)) && (number(1, 400) < ((GET_SKILL(victim, 
SKILL_SHIELD_BLOCK)*x)-encumberance_level(victim))) && (GET_POS(victim) == POS_FIGHTING)){
    act("&BYou skillfully &Cshield block&B $N attack!&n", FALSE, victim, 0, ch, TO_CHAR);
    act("&B$n skillfully &Cshield blocks&B your attack!&n", FALSE, victim, 0, ch, TO_VICT);
    act("&B$n skillfully &Cshield blocks&B $N attack!&n", FALSE, victim, 0, ch, TO_NOTVICT);
    improve_skill(victim, SKILL_SHIELD_BLOCK);
    return 1;
  }
  else if (AFF2_FLAGGED(victim, AFF2_BLINK) && (number(1, 500) <
GET_SKILL(victim, SPELL_BLINK))) {
    act("$N blinks away from your attack.", FALSE, ch, 0, victim, TO_CHAR);
    act("$N blinks out of $n's way.", FALSE, ch, 0, victim, TO_NOTVICT);
    act("You blink out of $n's way.", FALSE, ch, 0, victim, TO_VICT);
    return 1;
  }
 return 0;
}

void hit(struct char_data * ch, struct char_data * victim, int type)
{
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
  int w_type, victim_ac, calc_thaco, dam, diceroll;
  float str_fact ;   

  if (victim == NULL)
    return;

  if (AFF_FLAGGED(ch, AFF_HOLDED))
    return;

  if (!IS_NPC(victim) && type != SKILL_BACKSTAB)
     if (defender(ch, victim))
        return;

  /* check if the character has a fight trigger */
  fight_mtrigger(ch);

  if (ch->in_room != victim->in_room) {
    if (FIGHTING(ch) && FIGHTING(ch) == victim)
      stop_fighting(ch);
    return;
  }

  /* Find the weapon type (for display purposes only) */
  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
    w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
  else {
    if (IS_NPC(ch) && (ch->mob_specials.attack_type != 0))
      w_type = ch->mob_specials.attack_type + TYPE_HIT;
    else
      w_type = TYPE_HIT;
  }

  /* Calculate the THAC0 of the attacker */
  if (!IS_NPC(ch))
    calc_thaco = thaco((int) GET_CLASS(ch), (int) GET_LEVEL(ch));
  else          /* THAC0 for monsters is set in the HitRoll */
    calc_thaco = (18 - (GET_LEVEL(ch) / 9.9));

//  calc_thaco -= str_app[STRENGTH_APPLY_INDEX(ch)].tohit;
   calc_thaco -= GET_HITROLL(ch)/2;

   if (GET_STYLE(ch) == 4) // style savage -Ips & Str
	calc_thaco += number(5, 15);

   if (GET_STYLE(ch) == 5) // style aiming -Ips & Str
	calc_thaco -= number(5, 15);


   if ((wielded && IS_BLUDGEON(wielded)) && (GET_SKILL(ch, SKILL_BLUDGEON) > number(1, 94))) {
	   if (GET_SKILL(ch, SKILL_BLUDGEON_E) > number(1, 94)) {
		   calc_thaco -= GET_LEVEL(ch)/20;
		   improve_skill(ch, SKILL_BLUDGEON_E);
	   } else {
		   calc_thaco -= GET_LEVEL(ch)/24;
	   }
	   improve_skill(ch, SKILL_BLUDGEON);
   } else if ((wielded && IS_PIERCE(wielded)) && (GET_SKILL(ch, SKILL_PIERCE) > number(1, 94))) {
	   if (GET_SKILL(ch, SKILL_PIERCE_E) > number(1, 94)) {
		   calc_thaco -= GET_LEVEL(ch)/20;
		   improve_skill(ch, SKILL_PIERCE_E);
	   } else {
		   calc_thaco -= GET_LEVEL(ch)/24;
	   }
	   improve_skill(ch, SKILL_PIERCE);
   } else if ((wielded && IS_SLASH(wielded)) && (GET_SKILL(ch, SKILL_SLASH) > number(1, 94))) {
	   if (GET_SKILL(ch, SKILL_SLASH_E) > number(1, 94)) {
		   calc_thaco -= GET_LEVEL(ch)/20;
		   improve_skill(ch, SKILL_SLASH_E);
	   } else {
		   calc_thaco -= GET_LEVEL(ch)/24;
	   }
	   improve_skill(ch, SKILL_SLASH);
   }

	   /* Calculate the raw armor including magic armor.  Lower AC is better. */
  victim_ac = (GET_AC(victim) + prof_ac(ch) - (GET_DEX(ch) * 1.5f)) / 10;

  if (IS_AFFECTED(victim, AFF_BERZERK))
	victim_ac += 2;

  if ( FOREST(victim->in_room) && !IS_NPC(victim) ) { // -ips
	if ( GET_SKILL(victim, SKILL_FOREST_AGILITY) > number(1, 99) )
		victim_ac -= GET_LEVEL(victim)/40;
  }

  victim_ac = MAX(-100, victim_ac);      /* -10 is lowest */

  /* roll the die and take your chances... */
  diceroll = number(1, 20);

  /* decide whether this is a hit or a miss */
  if ((((diceroll < 20) && AWAKE(victim)) &&
       ((diceroll == 1) || ((calc_thaco - diceroll) > victim_ac)))) {
    /* the attacker missed the victim */
    if (type == SKILL_BACKSTAB)
      damage(ch, victim, 0, SKILL_BACKSTAB);
    else
      damage(ch, victim, 0, w_type);
    return;
  } else {
    /* okay, we know the guy has been hit.  now calculate damage. */

    /* Start with the damage bonuses: the damroll and strength apply */

    dam = GET_DAMROLL(ch)* 0.8; 

    /* Maybe holding arrow? */
    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
      /* Add weapon-based damage if a weapon is being wielded */
      dam += ((dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2))*GET_OBJ_COND(wielded))/100);
    } else {
      /* If no weapon, add bare hand damage instead */
      if (IS_NPC(ch)) {
	dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
      } else {
        dam += number(GET_LEVEL(ch) / 40, GET_LEVEL(ch) / 20);
      }
    }

 /* se ele tiver a hand damage skill aumenta o dano em no max 5 */
    if(!IS_NPC(ch)){
       if (!wielded && GET_SKILL(ch, SKILL_HAND_DAMAGE)){
          dam += 1 + (SKILL_HAND_DAMAGE / 50);
	  improve_skill(ch, SKILL_HAND_DAMAGE);
       }
    }

   if ((wielded && IS_BLUDGEON(wielded)) && (GET_SKILL(ch, SKILL_BLUDGEON_E) > number(1, 94))) {
	   if (GET_SKILL(ch, SKILL_BLUDGEON) > number(1, 94)) {
		   dam += GET_LEVEL(ch)/5 + GET_REMORT(ch)*4;
		   improve_skill(ch, SKILL_BLUDGEON);
	   } else {
		   dam += GET_LEVEL(ch)/8 + GET_REMORT(ch)*4;
	   }
	   improve_skill(ch, SKILL_BLUDGEON_E);
   } else if ((wielded && IS_PIERCE(wielded)) && (GET_SKILL(ch, SKILL_PIERCE_E) > number(1, 94))) {
	   if (GET_SKILL(ch, SKILL_PIERCE) > number(1, 94)) {
		   dam += GET_LEVEL(ch)/5 + GET_REMORT(ch)*4;
		   improve_skill(ch, SKILL_PIERCE);
	   } else {
		   dam += GET_LEVEL(ch)/8 + GET_REMORT(ch)*4;
	   }
	   improve_skill(ch, SKILL_PIERCE_E);
   } else if ((wielded && IS_SLASH(wielded)) && (GET_SKILL(ch, SKILL_SLASH_E) > number(1, 94))) {
	   if (GET_SKILL(ch, SKILL_SLASH) > number(1, 94)) {
		   dam += GET_LEVEL(ch)/5 + GET_REMORT(ch)*4;
		   improve_skill(ch, SKILL_SLASH);
	   } else {
		   dam += GET_LEVEL(ch)/8 + GET_REMORT(ch)*4;
	   }
	   improve_skill(ch, SKILL_SLASH_E);
   }

    /*
     * Include a damage multiplier if victim isn't ready to fight:
     *
     * Position sitting  1.33 x normal
     * Position resting  1.66 x normal
     * Position sleeping 2.00 x normal
     * Position stunned  2.33 x normal
     * Position incap    2.66 x normal
     * Position mortally 3.00 x normal
     *
     * Note, this is a hack because it depends on the particular
     * values of the POSITION_XXX constants.
     */
    if (GET_POS(victim) < POS_FIGHTING)
      dam *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;

    /* at least 1 hp damage min per hit */
    dam = MAX(1, dam);

	if (IS_AFFECTED(ch, AFF_BERZERK)) {
	    dam += number(1, MAX(1,(dam/4)));

            if(!IS_NPC(ch)) {
		if(GET_SKILL(ch,SKILL_IMPROVED_BERZERK)) {
			dam = dam * 1.5 ;
			improve_skill(ch, SKILL_IMPROVED_BERZERK) ;
                }
            }

        }

    if(GET_RACE(ch) == RACE_HILL_OGRE || (GET_RACE(ch) == RACE_DRACONIAN && number(1,12) ==
3)) dam = dam * 1.2 ;

    if(GET_RACE(victim) == RACE_NAUGRIM || (GET_RACE(victim) == RACE_DRACONIAN &&
number(1,12)== 3)) dam = MAX(1 , dam * 0.8) ;

   if(GET_RACE(ch) == RACE_DUNEDAIN) 
     dam = dam * 1.05 ;

   if(GET_RACE(victim) == RACE_DUNEDAIN) 
     dam =  MAX(1 , dam * 0.9) ;

   str_fact = (1+ ((float)GET_STR(ch) /100)) ; 
   dam = dam *str_fact    ;                  // cada ponto de forca aumenta
                                             // em 1% o dano, by Luigi.
    if (type == SKILL_BACKSTAB) {
      dam *= backstab_mult(GET_LEVEL(ch));
      if(GET_CLASS(ch) == CLASS_THIEF)
       dam = dam * 2 ;
      damage(ch, victim, dam, SKILL_BACKSTAB);
    }
    else if (type == SKILL_COMBO) {
      //if (!IS_NPC(victim)){
       dam *= 0.70;
       damage(ch, victim, dam, w_type);
    //}
      /*else{
       dam *= 0.90;
       damage(ch, victim, dam, w_type);}*/
       }
    else if (type == SKILL_SABRE) {
      //if (!IS_NPC(victim)){
       dam *= 0.80;
       damage(ch, victim, dam, w_type);
      //} else {
         //dam *= 1.0;
	 //damage(ch, victim, dam, w_type);
	 
    }
    else
      damage(ch, victim, dam, w_type);
  }

  /* check if the victim has a hitprcnt trigger */
 // hitprcnt_mtrigger(victim);
}

void hit2(struct char_data * ch, struct char_data * victim, int type)
{
  struct obj_data *wielded = GET_EQ(ch, WEAR_DWIELD);
  int w_type, victim_ac, calc_thaco, dam, diceroll;
  float str_fact ;   

  if (victim == NULL)
    return;
  if(!IS_NPC(victim) && (!IS_NPC(ch) && (PLR_FLAGGED(ch, PLR_NOPK)))
     && (!(ROOM_FLAGGED(ch->in_room, ROOM_ARENA))) && (!(ROOM_FLAGGED(ch->in_room, ROOM_ARENA)))){
    send_to_char("&wYou can't fight because you aren't a player killer...&n\r\n", ch);
    return;
  }
  if(!IS_NPC(ch) && (!IS_NPC(victim) && (PLR_FLAGGED(victim, PLR_NOPK)))
     && (!(ROOM_FLAGGED(ch->in_room, ROOM_ARENA))) && (!(ROOM_FLAGGED(ch->in_room, ROOM_ARENA)))){
    send_to_char("&wYou can't fight because your opponent isn't a player killer...&n\r\n", ch);
    return;
  }
  if(AFF_FLAGGED(ch, AFF_HOLDED))
    return;

  if(!IS_NPC(victim) && type != SKILL_BACKSTAB)
     if(defender(ch, victim))
        return;

  /* check if the character has a fight trigger */
  fight_mtrigger(ch);

  if (ch->in_room != victim->in_room) {
    if (FIGHTING(ch) && FIGHTING(ch) == victim)
      stop_fighting(ch);
    return;
  }

  /* Find the weapon type (for display purposes only) */
  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
    w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
  else {
    if (IS_NPC(ch) && (ch->mob_specials.attack_type != 0))
      w_type = ch->mob_specials.attack_type + TYPE_HIT;
    else
      w_type = TYPE_HIT;
  }

  /* Calculate the THAC0 of the attacker */
  if (!IS_NPC(ch))
    calc_thaco = thaco((int) GET_CLASS(ch), (int) GET_LEVEL(ch));
  else          /* THAC0 for monsters is set in the HitRoll */
    calc_thaco = (19 - (GET_LEVEL(ch) / 9.9));
//  calc_thaco -= str_app[STRENGTH_APPLY_INDEX(ch)].tohit;
  if (!IS_NPC(ch))
   calc_thaco -= GET_HITROLL(ch)/2;
  else
   calc_thaco -= GET_HITROLL(ch)/2;

   if (GET_STYLE(ch) == 4) // style savage -Ips & Str
	calc_thaco += number(5, 15);

   if (GET_STYLE(ch) == 5) // style aiming -Ips & Str
	calc_thaco -= number(5, 15);

  /* Calculate the raw armor including magic armor.  Lower AC is better. */
  victim_ac = (GET_AC(victim) + prof_ac(ch) - (GET_DEX(ch) * 1.5f)) / 10;
  if (IS_AFFECTED(victim, AFF_BERZERK))
	victim_ac += 2;

  victim_ac = MAX(-100, victim_ac);      /* -10 is lowest */

  /* roll the die and take your chances... */
  diceroll = number(1, 20);

  /* decide whether this is a hit or a miss */
  if ((((diceroll < 20) && AWAKE(victim)) &&
       ((diceroll == 1) || ((calc_thaco - diceroll) > victim_ac)))) {
    /* the attacker missed the victim */
    if (type == SKILL_BACKSTAB)
      damage(ch, victim, 0, SKILL_BACKSTAB);
    else
      damage(ch, victim, 0, w_type);
    return;
  } else {
    /* okay, we know the guy has been hit.  now calculate damage. */

    /* Start with the damage bonuses: the damroll and strength apply */
    dam = (GET_DAMROLL(ch))*0.8;

    if (wielded) {
      /* Add weapon-based damage if a weapon is being wielded */
      if (IS_NPC(ch)){
		  if ((dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2))) >  (dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice))){
			  dam += (((dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2)))*(GET_OBJ_COND(wielded)))/100);
       }
       else{
        dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
       }
      }
      else {
       dam += (((dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2)))*(GET_OBJ_COND(wielded)))/100);
      }
    } else {
      /* If no weapon, add bare hand damage instead */
      if (IS_NPC(ch)) {
        dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
      } else {
        dam += number(GET_LEVEL(ch) / 40, GET_LEVEL(ch) / 20);
      }
    }

 /* se ele tiver a hand damage skill aumenta o dano em no max 5 */
    if(!IS_NPC(ch)){
       if (!wielded && GET_SKILL(ch, SKILL_HAND_DAMAGE))
          dam += 1 + (SKILL_HAND_DAMAGE / 50);
	  improve_skill(ch, SKILL_HAND_DAMAGE);
    }

    /*
     * Include a damage multiplier if victim isn't ready to fight:
     *
     * Position sitting  1.33 x normal
     * Position resting  1.66 x normal
     * Position sleeping 2.00 x normal
     * Position stunned  2.33 x normal
     * Position incap    2.66 x normal
     * Position mortally 3.00 x normal
     *
     * Note, this is a hack because it depends on the particular
     * values of the POSITION_XXX constants.
     */
    if (GET_POS(victim) < POS_FIGHTING)
      dam *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;

    /* at least 1 hp damage min per hit */
    dam = MAX(1, dam);
	if (IS_AFFECTED(ch, AFF_BERZERK))
	    dam += number(1, MAX(1,(dam/4)) );

    if(GET_RACE(ch) == RACE_HILL_OGRE || (GET_RACE(ch) == RACE_DRACONIAN && number(1,12) ==
3)) dam = dam * 1.2 ;

    if(GET_RACE(victim) == RACE_NAUGRIM || (GET_RACE(victim) == RACE_DRACONIAN &&
number(1,12) == 3)) dam = MAX(1 , dam * 0.8) ;
 
    str_fact = (1+ ((float)GET_STR(ch) /100)) ;
    dam = dam *str_fact    ;                  // cada ponto de forca aumenta
                                             // em 1% o dano, by Luigi.
    if (type == SKILL_BACKSTAB) {
      dam *= backstab_mult(GET_LEVEL(ch));
      if(GET_CLASS(ch) == CLASS_THIEF)
       dam = dam * 2 ;

      damage(ch, victim, dam, SKILL_BACKSTAB);
    }
    else if (type == SKILL_COMBO) {
      dam *= 0.90;
      damage(ch, victim, dam, w_type);
    }
    else
      damage(ch, victim, dam, w_type);
  }

  /* check if the victim has a hitprcnt trigger */
// hitprcnt_mtrigger(victim);
}

#define MAX_STYLES	7

const char *estyles[] = {
/*-1*/     NULL,
/*0*/      "standard",
/*1*/      "defensive",
/*2*/      "offensive",
/*3*/      "pro",
/*4*/      "savage",
/*5*/      "aiming",
/*6*/      "battlecasting",
/*END*/    "\n"
};

ACMD(do_style)
{
  int x = 0;
  boolean t = FALSE;

  one_argument(argument, arg);

  if(!*arg) {
	sprintf(buf2, "&WYou current style is &g%s&W.&n\r\n", estyles[GET_STYLE(ch)+1]);
	send_to_char(buf2, ch);
	return;
  } else {
	for(x = 0; x < MAX_STYLES; x++)
	  if(isname(arg, estyles[x+1])) {
	     if(x == 3 && !GET_SKILL(ch, SKILL_STYLE_PRO)) {
	        send_to_char("You need to learn the pro style to fight like that.\r\n", ch);
		return;
	     }
	     GET_STYLE(ch) = x;
	     t = TRUE;
        }

	if(t) {
          sprintf(buf2, "&WYou changed your style to &g%s&W.&n\r\n", estyles[GET_STYLE(ch)+1]);
	  send_to_char(buf2, ch);
	} else
	  send_to_char("&WUsage: &Bstyle &W<&cdefensive&W/&Roffensive&W/&nstandard&W/&Cpro&W/&Ysavage&W/&Gaiming&W/&Mbattlecasting&W>&n\r\n", ch);
  }
}

/* 2 ruim, 1 medio, 0 bom */
int app_style[][3] = {
/*		DEFESA		ATAQUE		qqcoisa	*/
/*standard*/	{2, 		2,		 2},
/*standard*/	{1, 		1,		 2},
/*defensive*/	{0, 		2,		 1},
/*offensive*/	{2, 		0,		 3},
/*  pro	*/	{0, 		0,		 3},
/*savage*/	{2, 		0,		 3},
/* aiming*/	{1, 		1,		 3},
/*battlecast*/	{2, 		2,		 1}
};

void atacar(struct char_data *ch)
{
   sh_int max = 0, norm = 0, prob = 0, num = 0, at = 0;
   double x;

   max += GET_SKILL_LS(ch, SKILL_SECOND_ATTACK);
   max += GET_SKILL_LS(ch, SKILL_THIRD_ATTACK);
   max += GET_SKILL_LS(ch, SKILL_FOURTH_ATTACK);

   at += (GET_SKILL_LS(ch, SKILL_SECOND_ATTACK) ? 1 : 0);
   at += (GET_SKILL_LS(ch, SKILL_THIRD_ATTACK) ? 1 : 0);
   at += (GET_SKILL_LS(ch, SKILL_FOURTH_ATTACK) ? 1 : 0);

   norm = max/(app_style[GET_STYLE(ch)+1][1]+1);

   prob = number(0, max);
   prob += (IS_CARRYING_W(ch)/10);
   if(GET_EQ(ch, WEAR_WIELD))
     prob += GET_OBJ_WEIGHT(GET_EQ(ch, WEAR_WIELD));

   norm += (15 + con_app[GET_DEX(ch)].hitp)*2;

   if(AFF2_FLAGGED(ch, AFF2_SLOW))
   		norm *= 0.15;

   x = app_style[GET_STYLE(ch)+1][2];
   x /= 10;
   for(; at && norm > prob && num < app_style[GET_STYLE(ch)+1][2]; at--)
   {
       	hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
       	norm -= norm*x;
		improve_skill(ch, (SKILL_SECOND_ATTACK+num));
		num++;
   }
}

/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
  
  struct char_data *ch;
  struct follow_type *k;
  int perc, val;

  ACMD(do_assist);

  for (ch = combat_list; ch; ch = next_combat_list) {
    next_combat_list = ch->next_fighting;


    if (FIGHTING(ch) == NULL || ch->in_room != FIGHTING(ch)->in_room) {
      stop_fighting(ch);
      continue;
    }

    BASHTIME(ch) -= (BASHTIME(ch) > 0);

    if (IS_NPC(ch)) {
      if(contar_briga(ch) > 1)
      FIGHTING(ch) = melhor_alvo(ch);

      if (GET_MOB_WAIT(ch) > 0) {
        GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
        continue;
      }
      GET_MOB_WAIT(ch) = 0;

      if (GET_POS(ch) < POS_FIGHTING) {
        GET_POS(ch) = POS_FIGHTING;
        act("&w$U$n scrambles to $s feet!&n", TRUE, ch, 0, 0, TO_ROOM);
      }
    }

    if (GET_POS(ch) < POS_FIGHTING) {
      send_to_char("&WYou can't fight while sitting!!!&n\r\n", ch);
      continue;
    }

    /*mob_ia(ch);*/
    hit(ch, FIGHTING(ch), TYPE_UNDEFINED);

    for (k = ch->followers; k; k=k->next) {
      if (!IS_NPC(k->follower) && PRF_FLAGGED(k->follower, PRF_AUTOASSIST) &&
         (k->follower->in_room == ch->in_room) && GET_POS(k->follower) > POS_FIGHTING)
         do_assist(k->follower, GET_NAME(ch), 0, 0);
    }

    if (!IS_NPC(ch))
    {
      	atacar(ch);
      	if ((GET_EQ(ch, WEAR_DWIELD)) && GET_SKILL(ch, SKILL_DUAL_WIELD) && number(0, 
101) <= GET_SKILL(ch, SKILL_DUAL_WIELD) / 2 + (15 + con_app[GET_DEX(ch)].hitp))
      	{
      	  hit2(ch, FIGHTING(ch), TYPE_UNDEFINED);
			improve_skill(ch, SKILL_DUAL_WIELD);
      	}
    }

     if (IS_NPC(ch))
     {
	  perc = number(1, 101);
	  val = (AFF2_FLAGGED(ch, AFF2_SLOW) ? ch->mob_specials.attack1*0.85: ch->mob_specials.attack1);
	  if (val > perc)
            hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
	  perc = number(1,101);
	  val = (AFF2_FLAGGED(ch, AFF2_SLOW) ? ch->mob_specials.attack2*0.85: ch->mob_specials.attack2);
	  if (val > perc)
            hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
	  perc = number(1,101);
	  val = (AFF2_FLAGGED(ch, AFF2_SLOW) ? ch->mob_specials.attack3*0.85: ch->mob_specials.attack3);
	  if (val > perc)
            hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
     }

    if (AFF_FLAGGED(ch, AFF_HASTE)) {
		if (number(0, 101) < (15 + con_app[GET_DEX(ch)].hitp) * 2)
			hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
    }
    
        if (GET_EQ(ch, WEAR_WIELD) && IS_OBJ_STAT(( GET_EQ(ch, WEAR_WIELD)), ITEM_FIRE)){    	 
    	if (number(0, 101) < (15 + con_app[GET_DEX(ch)].hitp) * 1.5)
    	send_to_char("&RYou &Yb&Ru&Yr&Rn your enemy with a &Yf&Rl&Ya&Rm&Yi&Rn&Yg&R stroke of your weapon.&\r\n", ch);
    	hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
    	    	}

	if (GET_EQ(ch, WEAR_WIELD) && IS_OBJ_STAT(( GET_EQ(ch, WEAR_WIELD)), ITEM_WATER)){    	 
    	if (number(0, 101) < (15 + con_app[GET_DEX(ch)].hitp) * 1.5)
    	send_to_char("&WYou &CSky down &Wyour enemy with a powerful stroke of your weapon.&\r\n", ch);
    	hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
    	    	}


    if (!IS_NPC(ch) && GET_WIMP_LEV(ch) && (ch != FIGHTING(ch)) &&
	!IS_AFFECTED(ch, AFF_BERZERK) &&
        GET_HIT(ch) < GET_WIMP_LEV(ch) && GET_HIT(ch) > 0) {
      send_to_char("&WYou wimp out, and attempt to flee!&n\r\n", ch);
      do_flee(ch, NULL, 0, 0);
    }

    /* XXX: Need to see if they can handle "" instead of NULL. */
    if (MOB_FLAGGED(ch, MOB_SPEC) && mob_index[GET_MOB_RNUM(ch)].func != NULL)
      (mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, "");
   }
 }
ACMD(do_revive)
{
  int exp, min_exp = 0;
  struct affected_type af;

  if (IS_NPC(ch) || GET_LEVEL(ch) >= LVL_ELDER ||
      	!PLR_FLAGGED(ch, PLR_DEAD)){
	send_to_char("&WYou don't need this...&n\r\n", ch);
  	return;
  }

  exp = (level_exp(GET_REMORT(ch), (GET_LEVEL(ch) + 1))/2);
  if(GET_LEVEL(ch) > 10 && GET_EXP(ch) < exp){
    send_to_char("&WYou haven't reached the experience to revive, so the gods shoot a curse on you....&n\r\n", ch);

    af.type = SPELL_DAMNED_CURSE;
    af.location = APPLY_HITROLL;
    af.duration = 3;
    af.modifier = -5 - (GET_LEVEL(ch) / 30);
    af.bitvector = AFF_DAMNED_CURSE;
    af.bitvector2 = APPLY_NONE;
    af.bitvector3 = APPLY_NONE;
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);

    af.type = SPELL_DAMNED_CURSE;
    af.location = APPLY_DAMROLL;
    af.duration = 3;
    af.modifier = -5 - (GET_LEVEL(ch) / 30);
    af.bitvector = AFF_DAMNED_CURSE;
    af.bitvector2 = APPLY_NONE;
    af.bitvector3 = APPLY_NONE;
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);

  }

  send_to_char("\r\n&WYou feel your fisical body appearing again...&n\r\n"
	       "&WYour strenght is back to the normal, but your are so tired.&n\r\n"
               "&WYou are acquitted in the judgement.\r\nYou can be a mortal again.&n\r\n\r\n"
 	       "&WGood Luck, and enjoy WarDome.&n\r\n\r\n", ch);

  min_exp -= level_exp(GET_REMORT(ch), (GET_LEVEL(ch) + 1));
  if (PLR_FLAGGED(ch, PLR_KILLER)) {
  	GET_EXP(ch) = GET_EXP(ch);
 } else {
  if(GET_LEVEL(ch) > 1) {
    if(GET_EXP(ch)-exp >= min_exp)
      GET_EXP(ch) -= exp;
    else
      GET_EXP(ch) = min_exp;
  }
}

  GET_HIT(ch) = GET_MAX_HIT(ch) / 10;
  GET_MANA(ch) = GET_MAX_MANA(ch) / 10;
  GET_MOVE(ch) = GET_MAX_MOVE(ch) / 10;
//  GET_MENTAL(ch) = GET_MAX_MENTAL(ch) / 10;
  GET_OXI(ch) = GET_MAX_OXI(ch) / 10;

  REMOVE_BIT(PLR_FLAGS(ch), PLR_DEAD);
  REMOVE_BIT(PLR_FLAGS(ch), PLR_DEAD);

  if (ch->in_room != NOWHERE)
    char_from_room(ch);
    char_to_room(ch, real_room(500));
    act("&B$n &bappears in a strange wortex of light, created by the &BW&ba&Br&bD&Bo&bm&Be.&n", FALSE, ch, 0, 0, TO_NOTVICT);
    look_at_room(ch, 0);

    sprintf(buf, "(REVIVE) %s revives after %s dead.", GET_NAME(ch), (GET_SEX(ch) == SEX_MALE ? "his" : "her"));
    mudlog(buf, BRF, LVL_ELDER, TRUE);

  save_char(ch, ch->in_room);

}

/*int check_wanted(struct char_data * ch); //comentar essas 3 chamadas e a funcao abaixo se der algum erro
int check_wanted_gain_gold(struct char_data * ch);
int check_wanted_gain_qp(struct char_data * ch);

int return_wanted_gain(struct char_data * ch, struct char_data * victim)
{

	if(IS_NPC(ch) || IS_NPC(victim))
	   return FALSE;

	if(!check_wanted(victim))
	   return FALSE;

	if (ROOM_FLAGGED(ch->in_room, ROOM_ARENA) ||
		ROOM_FLAGGED(victim->in_room, ROOM_ARENA))
	   return FALSE;

	if(GET_CLAN(ch) >= 0) {
	  if(GET_CLAN(ch) == GET_CLAN(victim))
	   return FALSE;
	}

        if(!str_cmp(ch->desc->host, victim->desc->host)){
	  GET_POINTS(ch) -= 75;
  	  GET_POINTS(victim) -= 75;
	  GET_QP(ch) -= 75;
  	  GET_QP(victim) -= 75;
	  send_to_char("&WYou &Rlose &C75 &Bholy points&W and &C75 &Gquest points &Win this &Rtrick&W.&n\r\n", ch);
	  send_to_char("&WYou &Rlose &C75 &Bholy points&W and &C75 &Gquest points &Win this &Rtrick&W.&n\r\n", victim);
          return FALSE;
	}
	sprintf(buf, "You win %d questpoints and %s gold coins./r/n",
        check_wanted_gain_qp(victim), add_points(check_wanted_gain_gold(victim)));
        send_to_char(buf, ch);

        GET_QP(ch) += check_wanted_gain_qp(victim);
        GET_GOLD(ch) += check_wanted_gain_gold(victim);
	GET_KILLED(ch) = 0;
	GET_DIED_S(ch) += 1;

	return TRUE;
}*/

ACMD(do_pk)
{

	if(LIBERDADE(ch) > 0)
	{
		send_to_char("&RYou can be attacked now.&n\r\n", ch);
		LIBERDADE(ch) = 0;
		return;
	}


	if (PLR_FLAGGED(ch, PLR_SECPLAYER)){
		send_to_char("Second players can't be players killers.\r\n", ch);
		return;
    	}
	if(GET_REMORT(ch) > 1 || GET_LEVEL(ch) > 10){
		send_to_char("You are too experienced to choose.\r\n", ch);
		return;
	}

	if(GET_LEVEL(ch) < 10){
		send_to_char("You need be more experienced to choose.\r\n", ch);
		return;
	}

        REMOVE_BIT(PLR_FLAGS(ch), PLR_NOPK);
        send_to_char("&WBe Careful now. You are a &BPlayer &RKiller&W.\r\n", ch);

}



const int pk_remort_table[][2] = {
  {0, 0},
  {1, 1},
  {1, 2},
  {2, 4},
  {2, 6},
  {3, 8},
  {3, 10},
  {4, 12},
  {4, 14},
  {5, 99},
  {5, 99},
  {6, 99},
  {6, 99},
  {7, 99},
  {7, 99},
  {8, 99},
  {8, 99},
  {8, 99},
  {8, 99},
  {8, 99},
  {8, 99},
  {8, 99},
  {8, 99},
  {8, 99},
  {8, 99}
};

int new_can_pk(int chlvl, int vtlvl, int chr, int vtr)
{
	  int c, v, up, down;


  c = chlvl;
  v = vtlvl;

  if (c < 51) {
	up = c + 10;
	down = c - 10;
	if (v > up || v < down)
		return FALSE;
  } else if (c < 61) {
	up = c + 15;
	down = c - 10;
	if (v > up || v < down)
		return FALSE;
  } else if (c < 101) {
	up = c + 15;
	down = c - 15;
	if (v > up || v < down)
		return FALSE;
  } else if (c < 116) {
	up = c + 20;
	down = c - 15;
	if (v > up || v < down)
		return FALSE;
  } else if (c < 151) {
	up = c + 20;
	down = c - 20;
	if (v > up || v < down)
		return FALSE;
  } else if (c < 176) {
	up = c + 25;
	down = c - 20;
	if (v > up || v < down)
		return FALSE;
  } else if (c < 201) {
	up = c + 25;
	down = c - 25;
	if (v > up || v < down)
		return FALSE;
  }

  c = chr;
  v = vtr;

  if (v < pk_remort_table[c][0] || pk_remort_table[c][1] < v)
	return FALSE;
  return TRUE;
}


int can_pk(struct char_data * ch, struct char_data * vt)
{
  int c, v, up, down;

  if(ROOM_FLAGGED(ch->in_room, ROOM_ARENA) && ROOM_FLAGGED(vt->in_room, ROOM_ARENA))
	return TRUE;
  
  if(!IS_NPC(vt) && LIBERDADE(ch) > 0)
  	LIBERDADE(ch) = 0;

  if(LIBERDADE(vt))
  	return FALSE;

  if(FIGHTING(ch) == vt)
        return TRUE;

 if(PLR_FLAGGED(vt, PLR_THIEF)) //|| check_wanted(vt))
        return TRUE;
        
 if(PLR_FLAGGED(vt, PLR_KILLER))
        return TRUE;

  c = GET_LEVEL(ch);
  v = GET_LEVEL(vt);

  if (c < 51) {
	up = c + 10;
	down = c - 10;
	if (v > up || v < down)
		return FALSE;
  } else if (c < 61) {
	up = c + 15;
	down = c - 10;
	if (v > up || v < down)
		return FALSE;
  } else if (c < 101) {
	up = c + 15;
	down = c - 15;
	if (v > up || v < down)
		return FALSE;
  } else if (c < 116) {
	up = c + 20;
	down = c - 15;
	if (v > up || v < down)
		return FALSE;
  } else if (c < 151) {
	up = c + 20;
	down = c - 20;
	if (v > up || v < down)
		return FALSE;
  } else if (c < 176) {
	up = c + 25;
	down = c - 20;
	if (v > up || v < down)
		return FALSE;
  } else if (c < 201) {
	up = c + 25;
	down = c - 25;
	if (v > up || v < down)
		return FALSE;
  }

  c = GET_REMORT(ch);
  v = GET_REMORT(vt);

  if (v < pk_remort_table[c][0] || pk_remort_table[c][1] < v)
	return FALSE;

  if (PLR_FLAGGED(ch, PLR_NOPK) || PLR_FLAGGED(vt, PLR_NOPK))
        return FALSE;
        
    if((GET_CLAN(ch) && GET_CLAN(vt)) > 0)
  { 
   	if((clans[posicao_clan(GET_CLAN(ch))].em_guerra || clans[posicao_clan(GET_CLAN(vt))].em_guerra) == -1)
     	return FALSE;
     	
    	if((clans[posicao_clan(GET_CLAN(ch))].alianca || clans[posicao_clan(GET_CLAN(vt))].alianca) == -1)
 	return FALSE;
 	
 	if(clans[posicao_clan(GET_CLAN(ch))].em_guerra == GET_CLAN(vt) && clans[posicao_clan(GET_CLAN(vt))].em_guerra == GET_CLAN(ch)) 
	return TRUE;

  	if(clans[posicao_clan(GET_CLAN(ch))].em_guerra == GET_CLAN(vt) && clans[posicao_clan(GET_CLAN(vt))].em_guerra == clans[posicao_clan(GET_CLAN(ch))].alianca) 
	return TRUE;
  	
  	if(clans[posicao_clan(GET_CLAN(ch))].em_guerra == clans[posicao_clan(GET_CLAN(vt))].alianca && clans[posicao_clan(GET_CLAN(vt))].em_guerra == GET_CLAN(ch)) 
	return TRUE;
  	
  	if(clans[posicao_clan(GET_CLAN(ch))].alianca == GET_CLAN(vt) && clans[posicao_clan(GET_CLAN(vt))].alianca == GET_CLAN(ch)) 
	return FALSE;
   } 
  return FALSE;
}

