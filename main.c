#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>
#include <t3d/t3ddebug.h>

#include "emotions.h"

bool coop = true;
int mapnumber = 0;

float exposure = 5;

rdpq_font_t* font;
rdpq_font_t* font2;

#define T3D_TOUNITS(x) (6.4f*x)
#define T3D_FROMUNITS(x) (x*(1.0/64.0f))

#define TPE_TOUNITS(x) (512.0f*x)
#define TPE_FROMUNITS(x) (x*(1.0/512.0f))

const char* bar_matqueue_a[] = {
    "A_floortile_a",
    "B_floortileers",
    "CA_door01",
    "CB_wall01",
    "zbuffered_vertex",
    "D_bricks01",
    "E_tilesa",
    "G_wood01",
    "H_foliage",
    "J_bottles",
    "K_lamps",
    "zbuffered_vertex_b",
};
int bar_matqueue_a_count = 12;

const char* bar_matqueue_b[] = {
    "F_tiles_b",
    "G_wood_b",
    "J_bottles_b",
    "zbuffered_vertex_c",
    "zbuffered_vertex_d"
};
int bar_matqueue_b_count = 5;

inline int iwrap(int x, float min, float max) {
    if(x > max) return min;
    if(x < min) return max;
    return x;
}

int maxmap = 0;
bool show_a_sprite = false;

#define SHORTSTR_LENGTH 32

#define AUDIO_CHANNEL_MUSIC 4
#define AUDIO_CHANNEL_SOUND 0
#define AUDIO_SOUND_MAXSOUNDS 2

wav64_t bgmusic[24];
wav64_t sounds[32];
char bgmusicnames[64][32];
char soundsnames[64][32];

int musiccount = 0;
int soundscount = 0;

int sound_channel = 0;

bool bgmusic_playing;
bool sound_playing;
char bgmusic_name[SHORTSTR_LENGTH];
char sound_name[SHORTSTR_LENGTH];

float bgmusic_vol = 1;
float sound_vol = 1;

int transitionstate;
float transitiontime;
float transitiontimemax;
bool loopingmusic;

/// @brief Should be called on each game tick for audio to be played (between rdpq_attach and rdpq_detach)
void audioutils_mixer_update();

void audio_prewarm_all();

/// @brief Play music in the background
/// @param name fn of the music in the bgm folder
/// @param loop is the music looped
/// @param transition transition in seconds
void bgm_play(const char* name, bool loop, float transition);

/// @brief Stop the currently playing music
/// @param transition transition in seconds
void bgm_stop(float transition);

/// @brief Play sound effect
/// @param name fn of the sound in the sfx folder
/// @param loop is the music looped
void sound_play(const char* name, bool loop);

/// @brief Stop the currently playing music
void sound_stop();

/// @brief Set the music volume
/// @param vol 0-1 range
void music_volume(float vol);

/// @brief Set the sounds volume
/// @param vol 0-1 range
void sound_volume(float vol);

/// @brief Get the music current volume
/// @return 0-1 float
float music_volume_get();

/// @brief Get the sounds current volume
/// @return 0-1 float
float sound_volume_get();

int audio_prewarm_all_sounds_callback(const char *fn, dir_t *dir, void *data){
    char nameonly[128] = {0};
    strcpy(nameonly, strrchr(fn, '/') + 1);
    *strrchr(nameonly, '.') = '\0';
    strcpy(soundsnames[soundscount], nameonly);
    
    debugf("Found sound %i: %s  | filename %s\n", soundscount, nameonly, fn);
    wav64_open(&sounds[soundscount], fn);

    soundscount++;
    return DIR_WALK_CONTINUE;
}

int audio_prewarm_all_music_callback(const char *fn, dir_t *dir, void *data){

    char nameonly[128] = {0};
    strcpy(nameonly, strrchr(fn, '/') + 1);
    *strrchr(nameonly, '.') = '\0';
    strcpy(bgmusicnames[musiccount], nameonly);
    debugf("Found music %i: %s  | filename %s\n", musiccount, nameonly, fn);
    wav64_open(&bgmusic[musiccount], fn);

    musiccount++;
    return DIR_WALK_CONTINUE;
}


void audio_prewarm_all(){
    dir_glob("**/*.wav64", "rom:/music/", audio_prewarm_all_music_callback, NULL);
    dir_glob("**/*.wav64", "rom:/sfx/", audio_prewarm_all_sounds_callback, NULL);
}

int audio_find_sound(const char* name){
    int index = 0;
    while(strcmp(soundsnames[index], name) && index < 32) index++;
    if(index >= 32) assertf(0, "Sound not found %s", name);
    return index;
}

int audio_find_music(const char* name){
    int index = 0;
    while(strcmp(bgmusicnames[index], name) && index < 24) index++;
    if(index >= 24) assertf(0, "Music not found %s", name);
    return index;
}

void audioutils_mixer_update(){
    float volume = bgmusic_vol;
    mixer_try_play();
    if( transitionstate == 1 &&  transitiontime > 0){
         transitiontime -= display_get_delta_time();

        if( transitiontime <= 0) {
             transitionstate = 2;
            if( bgmusic_playing){
                 bgmusic_playing = false;
                mixer_ch_stop(AUDIO_CHANNEL_MUSIC);
                rspq_wait();
            }
            if( bgmusic_name[0]){
                //char fn[512]; sprintf(fn, "rom:/music/%s.wav64",  bgmusic_name);
                //wav64_open(&bgmusic, fn);
                wav64_t* mus = &bgmusic[audio_find_music(bgmusic_name)];
                wav64_set_loop(mus,  loopingmusic);
                wav64_play(mus, AUDIO_CHANNEL_MUSIC);
                bgmusic_playing = true;
            }
        }
        volume *= ( transitiontime /  transitiontimemax);
    }
    if( transitionstate == 2 &&  transitiontime <  transitiontimemax){
         transitiontime += display_get_delta_time();
        volume *= ( transitiontime /  transitiontimemax);
    }
    mixer_ch_set_vol(AUDIO_CHANNEL_MUSIC, volume * 0.65f, volume * 0.65f);
}

void bgm_hardplay(const char* name, bool loop, float transition){
     loopingmusic = loop;
     transitionstate = 0;
     transitiontime = 0;
     transitiontimemax = 1;
    wav64_t* mus = &bgmusic[audio_find_music(name)];
    wav64_set_loop(mus,  loop);
    wav64_play(mus, AUDIO_CHANNEL_MUSIC);
     bgmusic_playing = true;
    strcpy( bgmusic_name, name);
}

void bgm_play(const char* name, bool loop, float transition){
    if(transition == 0) { bgm_hardplay(name, loop, transition); return;}
    bgm_stop(1);
     loopingmusic = loop;
     transitionstate = 1;
     transitiontime = transition;
     transitiontimemax = transition;
    strcpy( bgmusic_name, name);
}


void bgm_hardstop(){
     bgmusic_playing = false;
     transitionstate = 0;
     transitiontime = 0.1;
     transitiontimemax = 0.1;
     bgmusic_name[0] = 0;
    mixer_ch_stop(AUDIO_CHANNEL_MUSIC);
    rspq_wait();
}

void bgm_stop(float transition){
    if(transition == 0) { bgm_hardstop(); return;}
     transitionstate = 1;
     transitiontime = 1;
     transitiontimemax = 1;
     bgmusic_name[0] = 0;
}

void sound_play(const char* name, bool loop){
    sound_stop();
    wav64_t* snd = &sounds[audio_find_sound(name)];
    wav64_set_loop(snd,  loop);
    wav64_play(snd, AUDIO_CHANNEL_SOUND + sound_channel*2);
    mixer_ch_set_vol(AUDIO_CHANNEL_SOUND + sound_channel*2,  sound_vol,  sound_vol);
    sound_channel++;
    if(sound_channel >= AUDIO_SOUND_MAXSOUNDS) sound_channel = 0;
     sound_playing = true;
    strcpy( sound_name, name);
}

void sound_stop(){
    if( sound_playing){
        mixer_ch_stop(AUDIO_CHANNEL_SOUND + sound_channel*2);
    }
     sound_playing = false;
     sound_name[0] = 0;
}

void music_volume(float vol){
    bgmusic_vol = vol;
}

void sound_volume(float vol){
    sound_vol = vol;
    for(int ch = AUDIO_CHANNEL_SOUND; ch < AUDIO_CHANNEL_SOUND + 2*AUDIO_SOUND_MAXSOUNDS; ch += 2){
      mixer_ch_set_vol(ch, vol, vol);
    }
}

float music_volume_get() {return  bgmusic_vol;};

float sound_volume_get() {return  sound_vol;};

#define MAXPLAYERS 2

typedef struct effectdata_s{

    float rumbletime[MAXPLAYERS];
    bool  rumblestate[MAXPLAYERS];
} effectdata_t;

effectdata_t effects;

void effects_update(){
    for(int i = 0; i < MAXPLAYERS; i++){
        if(effects.rumbletime[i] > 0){
            effects.rumbletime[i] -= display_get_delta_time();
            if(!effects.rumblestate[i]) {joypad_set_rumble_active(i, true); effects.rumblestate[i] = true;}
        } else if(effects.rumblestate[i]) {joypad_set_rumble_active(i, false); effects.rumblestate[i] = false;}
    }
}

void effects_rumble_stop(){
    for(int i = 0; i < MAXPLAYERS; i++){
        joypad_set_rumble_active(i, false);
        effects.rumblestate[i] = false;
    }
}

void effects_add_rumble(joypad_port_t port, float time){
    if(port < 0) return;
    effects.rumbletime[port] += time;
}

typedef struct {
    const char* bg;
    const char* text;
    int style;
} comic_entry_t;

comic_entry_t logo_intro[] = {
    { "rom:/UI/logo.ci8.sprite", "", 5 }
};

comic_entry_t comics_intro[] = {
    { "rom:/UI/intro1.ci8.sprite", "Двое амбициозных инопланетян на космическом корабле бороздили просторы вселенной в поисках планеты, которую они бы могли захватить для своих творческих экспериментов.", 0 },
    { "rom:/UI/intro2.ci8.sprite", "Они наблюдают за жителями этой планеты и узнают, что те крайне эмоциональные существа. Эта странная вещь - эмоция - кажется, способна так сильно на них воздействовать, что те буквально теряют над собой контроль. ", 0 },
    { "rom:/UI/intro2.ci8.sprite", "К тому же любопытные захватчики узнают, что люди очень любят проводить время в неком месте под названием «бар». Там они этот переизбыток эмоций глушат, и, похоже, только рады этому. ", 0 },
    { "rom:/UI/intro2.ci8.sprite", "Нашим захватчикам приходит невероятная идея постичь искусство управления эмоциями. Это ведь такой необычный способ захватить чужую планету!", 0 },
    { "rom:/UI/intro3.ci8.sprite", "Используя свои передовые технологии они без проблем определяют две базовые эмоции, которые им удается синтезировать в жидком виде. Дело за малым - начать полевые эксперименты! А там, глядишь, и новые удастся синтезировать. Для более тонкой промывки мозгов.", 0 },
    { "rom:/UI/intro4.ci8.sprite", "Они приземляются на Землю, и корабль входит в режим маскировки. Неопознанный летающий объект быстро становится вполне себе опознаваемым привлекательным баром, а наши инопланетные захватчики готовятся к открытию.", 0 },
    { "rom:/UI/intro4.ci8.sprite", "Им не терпится начать постигать искусство бариста эмоций и изучить первого испытуемого посредством непринужденной беседы. Цель изучения — определение и нейтрализация человеческих эмоций!", 0 },
};

comic_entry_t level1_intro[] = {
    { "rom:/UI/tutorial1.ci8.sprite", "Туториал: К вам будут приходить посетители желающие избавиться от эмоций. Из беседы можно узнать о их проблемах и как сильно они испытывают те или иные эмоции. Жмите А чтобы говорить с ними.", 0 },
    { "rom:/UI/tutorial2.ci8.sprite", "Туториал: Вам нужно будет сделать смесь нейтрализующую эмоции клиента. Чем больше конкретной эмоции чувствует клиент, тем больше антиэмоции потребуется!\nДля начала нужно будет взять и поставить бокал на стойку. Жмите А чтобы взять бокал и у стойки нажмите А еще раз чтобы поставить его.", 0 },
    { "rom:/UI/tutorial3.ci8.sprite", "Туториал: Затем возьмите подходящие ингредиенты (эмоции) на полке. Жмите А чтобы взять предмет и B чтобы убрать его. Следите за его количеством!", 0 },
    { "rom:/UI/tutorial4.ci8.sprite", "Туториал: Смешивайте в бокал антиэмоции и нейтрализуйте всё что посетитель чувствует. Пусть они станут безэмоциональными и послушными вам!", 0 },
    { "rom:/UI/tutorial5.ci8.sprite", "Готовы? Тогда начинаем через 3... 2... 1...", 0 }
};

comic_entry_t level2_intro[] = {
    { "rom:/UI/dialogue.ci8.sprite", "Паньши: Невероятно, правда? Человеки даже не удивились, когда наш бар вдруг появился из ниоткуда!", 5 },
    { "rom:/UI/dialogue.ci8.sprite", "Сеньши: Да, это потрясающе! К тому же наша разработка жидких «анти-эмоций» сработала лучше, чем мы ожидали. ", 6 },
    { "rom:/UI/dialogue.ci8.sprite", "Паньши: И мы умудрились стать отличными баристами! Какой же это удобный способ захвата планеты, оказывается. ", 5 },
    { "rom:/UI/dialogue.ci8.sprite", "Сеньши: Именно. А благодаря близкому контакту мы смогли синтезировать новые растворители эмоций — для страха и злости. ", 6},
    { "rom:/UI/dialogue.ci8.sprite", "Паньши: Ух ты! Завтра обещает быть интересным. Кажется, мы сможем провести гораздо более сложные эксперименты!", 5 },
    { "rom:/UI/dialogue.ci8.sprite", "Готовы к уровню 2? Тогда начинаем через 3... 2... 1...", 0 }
};

comic_entry_t level3_intro[] = {
    { "rom:/UI/dialogue.ci8.sprite", "Паньши: Работать с четырьмя растворителями эмоций оказалось настоящим испытанием. ", 5 },
    { "rom:/UI/dialogue.ci8.sprite", "Сеньши: Да, но не зря же мы сдавали экзамены на захватчиков планет! Мы готовы справляться с любыми трудностями! ", 6 },
    { "rom:/UI/dialogue.ci8.sprite", "Паньши: Верно! Кстати, коктейли с четырьмя ингредиентами показали отличные результаты. Мы уже ближе к нашей цели. ", 5 },
    { "rom:/UI/dialogue.ci8.sprite", "Сеньши: И бар стал очень популярным среди людей! Все идет как по маслу. Точнее шло бы, не кончайся у нас запас «анти-эмоций»... ", 6},
    { "rom:/UI/dialogue.ci8.sprite", "Паньши: Поэтому нужно пополнить запасы экспериментальными ингредиентами. ", 5 },
    { "rom:/UI/dialogue.ci8.sprite", "Сеньши: Ты про то, что даже обычные вещи вызывают у людей сильные эмоции? Это можно использовать.", 6},
    { "rom:/UI/dialogue.ci8.sprite", "Паньши: Нужно использовать всё, что под рукой! Это точно поможет нам в осуществлении плана! ", 5 },
    { "rom:/UI/tutorial6.ci8.sprite", "Туториал: В баре появились различные предметы! Каждый предмет вызывает определенные эмоции! Берите их нажимая А и добавляйте их в смеси если закончились бутылки.", 0 },
    { "rom:/UI/dialogue.ci8.sprite", "Готовы к уровню 3? Тогда начинаем через 3... 2... 1...", 0 }
};

comic_entry_t level4_intro[] = {
    { "rom:/UI/dialogue.ci8.sprite", "Паньши: Теория подтверждена! Даже простые предметы могут влиять на эмоции людей.", 5 },
    { "rom:/UI/dialogue.ci8.sprite", "Сеньши: Это настоящий прорыв для нашей захватнической деятельности!", 6 },
    { "rom:/UI/dialogue.ci8.sprite", "Паньши: Совсем скоро эта планета станет готова к тому, чтобы показать ее нашей колонии.", 5 },
    { "rom:/UI/dialogue.ci8.sprite", "Сеньши: А бар всё больше набирает популярность! Кстати, был у нас тут посетитель... Большая шишка! Обещал этой ночью устроить у нас большую вечеринку. Будет много работы.", 6},
    { "rom:/UI/dialogue.ci8.sprite", "Паньши: Это здорово, больше подопытных! Но... есть и проблемы — растворителя эмоций осталось очень мало.", 5 },
    { "rom:/UI/dialogue.ci8.sprite", "Сеньши: Да, нам нужно будет внимательно слушать посетителей и готовить коктейли. У нас нет права на ошибки!", 6},
    { "rom:/UI/dialogue.ci8.sprite", "Готовы к последнему уровню? Тогда начинаем через 3... 2... 1...", 0 }
};

comic_entry_t outro_comic[] = {
    { "rom:/UI/outro1.ci8.sprite", "Коварный план двух изобретательных пришельцев удался. Хоть это и невозможно с точки зрения науки, они в совершенстве отточили искусство определения эмоций людей с последствующим подавлением. ", 0 },
    { "rom:/UI/outro2.ci8.sprite", "Прежние хозяева Земли стали безвольными пустышками, оставив свои амбиции в инопланетном баре. Всё готово для успешного захвата! Сигнал в колонию был отправлен и сородичи уже спешат к нашим захватчикам, чтобы присоединиться к ним.", 0 },
    { "rom:/UI/outro2.ci8.sprite", "Спасибо за игру!", 0 },
};

comic_entry_t credits_comic[] = {
    { "rom:/UI/credits.ci8.sprite", "Бар Антимоция", 0 },
    { "rom:/UI/intro4.ci8.sprite", "Программирование и сборка\n\nSpooky Илюха", 0 },
    { "rom:/UI/credits.ci8.sprite", "Арт и сюжет\n\nFeijoatl", 0 },
    { "rom:/UI/outro2.ci8.sprite", "Использованные утилиты\n\nLibdragon\nTiny3D\nBlender", 0 },
    { "rom:/UI/credits.ci8.sprite", "Использованные ресурсы\n\nfreesound.org\nmodarchive.org\npolyhaven.com", 0 },
    { "rom:/UI/logo.ci8.sprite", "", 0 },
};

sprite_t* background;
rspq_block_t* background_block;

void show_comic(comic_entry_t comic[], int count){
    for(int i = 0; i < count; i++){
        float logotime = 0;
        if(background_block) {rspq_block_free(background_block); background_block = NULL;}
        if(background) {sprite_free(background); background = NULL;}
        background = sprite_load(comic[i].bg);
        while(logotime < 11){
            rdpq_attach(display_get(), NULL);
            rdpq_set_scissor(0,0, 640, 480);
            if(display_interlace_rdp_field() >= 0) 
             rdpq_enable_interlaced(display_interlace_rdp_field());
            else rdpq_disable_interlaced();
            joypad_poll();
            effects_update();
            audioutils_mixer_update();
            joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);

            if(pressed.a || pressed.b || pressed.start) logotime = 11;
            float modulate = logotime < 1? logotime * 250 : 250;
            if(logotime > 10.1f) modulate = (11.1f - logotime) * 250;
            rdpq_set_prim_color(RGBA32(modulate,modulate,modulate,255));
            if(!background_block){
                rspq_block_begin();
                rdpq_set_mode_standard();
                rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT);
                rdpq_mode_filter(FILTER_BILINEAR);
                rdpq_mode_dithering(DITHER_BAYER_INVBAYER);
                rdpq_sprite_blit(background,0,0,NULL);
                background_block = rspq_block_end();
            } rspq_block_run(background_block);

            rdpq_textparms_t textparms = {0};
            textparms.width = 550;
            textparms.height = 200;
            textparms.align = ALIGN_LEFT; textparms.valign = VALIGN_CENTER;
            textparms.wrap = WRAP_WORD;
            textparms.style_id = comic[i].style;
            rdpq_text_printf(&textparms, 3, 50, 300, comic[i].text);
            audioutils_mixer_update();
            rdpq_detach_show();
            logotime += display_get_delta_time();
        }
        rdpq_attach(display_get(), NULL);
        rdpq_set_scissor(0,0, 640, 480);
        if(display_interlace_rdp_field() >= 0) 
             rdpq_enable_interlaced(display_interlace_rdp_field());
        else rdpq_disable_interlaced();
        audioutils_mixer_update();
        rdpq_clear(RGBA32(0,0,0,0));
        rdpq_detach_show();
        rspq_wait();
        if(background_block) {rspq_block_free(background_block); background_block = NULL;}
        if(background) {sprite_free(background); background = NULL;}
    }   
}


void show_comic_credits(comic_entry_t comic[], int count){
    for(int i = 0; i < count; i++){
        float logotime = 0;
        if(background_block) {rspq_block_free(background_block); background_block = NULL;}
        if(background) {sprite_free(background); background = NULL;}
        background = sprite_load(comic[i].bg);
        while(logotime < 11){
            rdpq_attach(display_get(), NULL);
            rdpq_set_scissor(0,0, 640, 480);
            if(display_interlace_rdp_field() >= 0) 
             rdpq_enable_interlaced(display_interlace_rdp_field());
            else rdpq_disable_interlaced();
            joypad_poll();
            effects_update();
            audioutils_mixer_update();
            joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);

            if(pressed.a || pressed.b || pressed.start) logotime = 11;
            float modulate = logotime < 1? logotime * 250 : 250;
            if(logotime > 10.1f) modulate = (11.1f - logotime) * 250;
            rdpq_set_prim_color(RGBA32(modulate,modulate,modulate,255));
            if(!background_block){
                rspq_block_begin();
                rdpq_set_mode_standard();
                rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT);
                rdpq_mode_filter(FILTER_BILINEAR);
                rdpq_mode_dithering(DITHER_BAYER_INVBAYER);
                rdpq_sprite_blit(background,0,0,NULL);
                background_block = rspq_block_end();
            } rspq_block_run(background_block);

            rdpq_textparms_t textparms = {0};
            textparms.width = 550;
            textparms.height = 200;
            textparms.align = ALIGN_CENTER; textparms.valign = VALIGN_CENTER;
            textparms.wrap = WRAP_WORD;
            textparms.style_id = comic[i].style;
            rdpq_text_printf(&textparms, 2, 50, 100, comic[i].text);
            audioutils_mixer_update();
            rdpq_detach_show();
            logotime += display_get_delta_time();
        }
        rdpq_attach(display_get(), NULL);
        rdpq_set_scissor(0,0, 640, 480);
        if(display_interlace_rdp_field() >= 0) 
             rdpq_enable_interlaced(display_interlace_rdp_field());
        else rdpq_disable_interlaced();
        audioutils_mixer_update();
        rdpq_clear(RGBA32(0,0,0,0));
        rdpq_detach_show();
        rspq_wait();
        if(background_block) {rspq_block_free(background_block); background_block = NULL;}
        if(background) {sprite_free(background); background = NULL;}
    }   
}

bool enable_infinite_mode(){
        bool selected = false; 
        bool result = false;
        while(!selected){
            rdpq_attach(display_get(), NULL);
            rdpq_set_scissor(0,0, 640, 480);
            if(display_interlace_rdp_field() >= 0) 
             rdpq_enable_interlaced(display_interlace_rdp_field());
            else rdpq_disable_interlaced();
            rdpq_clear(RGBA32(0,0,0,0));

            joypad_poll();
            effects_update();
            audioutils_mixer_update();
            joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);

            if(pressed.a) {
              selected = true;
              result = true;
            } else if (pressed.b){
              selected = true;
              result = false;
            }

            rdpq_textparms_t textparms = {0};
            textparms.width = 550;
            textparms.height = 200;
            textparms.align = ALIGN_CENTER; textparms.valign = VALIGN_CENTER;
            textparms.wrap = WRAP_WORD;
            textparms.style_id = 0;
            rdpq_text_printf(&textparms, 2, 50, 100, "Доступен бескоенчный режим! Играть?\n\nA - Начать\nB - В меню");
            audioutils_mixer_update();
            rdpq_detach_show();
        }
        rspq_wait();

        return result;
        
}

void libdragon_logo()
{
      rspq_wait();
    const color_t RED = RGBA32(221, 46, 26, 255);
    const color_t WHITE = RGBA32(255, 255, 255, 255);

    sprite_t *d1 = sprite_load("rom:/UI/dragon1.i8.sprite");
    sprite_t *d2 = sprite_load("rom:/UI/dragon2.i8.sprite");
    sprite_t *d3 = sprite_load("rom:/UI/dragon3.i8.sprite");
    sprite_t *d4 = sprite_load("rom:/UI/dragon4.i8.sprite");
    wav64_t music;
    wav64_open(&music, "rom:/sfx/dragon.wav64");
    mixer_ch_set_limits(0, 0, 48000, 0);
    

    display_init(RESOLUTION_640x480, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);

    
    float angle1 = 0, angle2 = 0, angle3 = 0;
    float scale1 = 0, scale2 = 0, scale3 = 0, scroll4 = 0;
    uint32_t ms0 = 0;
    int anim_part = 0;
    const int X0 = 10, Y0 = 30; // translation offset of the animation (simplify centering)

    void reset() {
        ms0 = get_ticks_ms();
        anim_part = 0;

        angle1 = 3.2f;
        angle2 = 1.9f;
        angle3 = 0.9f;
        scale1 = 0.0f;
        scale2 = 0.4f;
        scale3 = 0.8f;
        scroll4 = 400;
        wav64_play(&music, 0);
    }

    reset();
    while (1) {
        mixer_try_play();
        
        // Calculate animation part:
        // 0: rotate dragon head
        // 1: rotate dragon body and tail, scale up
        // 2: scroll dragon logo
        // 3: fade out 
        uint32_t tt = get_ticks_ms() - ms0;
        if (tt < 1000) anim_part = 0;
        else if (tt < 1500) anim_part = 1;
        else if (tt < 4000) anim_part = 2;
        else if (tt < 5000) anim_part = 3;
        else break;

        // Update animation parameters using quadratic ease-out
        angle1 -= angle1 * 0.04f; if (angle1 < 0.010f) angle1 = 0;
        if (anim_part >= 1) {
            angle2 -= angle2 * 0.06f; if (angle2 < 0.01f) angle2 = 0;
            angle3 -= angle3 * 0.06f; if (angle3 < 0.01f) angle3 = 0;
            scale2 -= scale2 * 0.06f; if (scale2 < 0.01f) scale2 = 0;
            scale3 -= scale3 * 0.06f; if (scale3 < 0.01f) scale3 = 0;
        }
        if (anim_part >= 2) {
            scroll4 -= scroll4 * 0.08f;
        }

        // Update colors for fade out effect
        color_t red = RED;
        color_t white = WHITE;
        if (anim_part >= 3) {
            red.a = 255 - (tt-4000) * 255 / 1000;
            white.a = 255 - (tt-4000) * 255 / 1000;
        }

        #if 0
        // Debug: re-run logo animation on button press
        joypad_poll();
        joypad_buttons_t btn = joypad_get_buttons_pressed(JOYPAD_PORT_1);
        if (btn.a) reset();
        #endif

        surface_t *fb = display_get();
        rdpq_attach_clear(fb, NULL);

        // To simulate the dragon jumping out, we scissor the head so that
        // it appears as it moves.
        if (angle1 > 1.0f) {
            // Initially, also scissor horizontally, 
            // so that the head tail is not visible on the right.
            rdpq_set_scissor(0, 0, X0+300, Y0+240);    
        } else {
            rdpq_set_scissor(0, 0, 640, Y0+240);
        }

        // Draw dragon head
        rdpq_set_mode_standard();
        rdpq_mode_alphacompare(1);
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,PRIM),(TEX0,0,PRIM,0)));
        rdpq_set_prim_color(red);
        rdpq_sprite_blit(d1, X0+216, Y0+205, &(rdpq_blitparms_t){ 
            .theta = angle1, .scale_x = scale1+1, .scale_y = scale1+1,
            .cx = 176, .cy = 171,
        });

        // Restore scissor to standard
        rdpq_set_scissor(0, 0, 640, 480);

        // Draw a black rectangle with alpha gradient, to cover the head tail
        rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
        rdpq_mode_dithering(DITHER_NOISE_NOISE);
        float vtx[4][6] = {
            //  x,      y,    r,g,b,a
            { X0+0,   Y0+180, 0,0,0,0 },
            { X0+200, Y0+180, 0,0,0,0 },
            { X0+200, Y0+240, 0,0,0,1 },
            { X0+0,   Y0+240, 0,0,0,1 },
        };
        rdpq_triangle(&TRIFMT_SHADE, vtx[0], vtx[1], vtx[2]);
        rdpq_triangle(&TRIFMT_SHADE, vtx[0], vtx[2], vtx[3]);

        if (anim_part >= 1) {
            // Draw dragon body and tail
            rdpq_set_mode_standard();
            rdpq_mode_alphacompare(1);
            rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
            rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,PRIM),(TEX0,0,PRIM,0)));

            // Fade them in
            color_t color = red;
            color.r *= 1-scale3; color.g *= 1-scale3; color.b *= 1-scale3;
            rdpq_set_prim_color(color);

            rdpq_sprite_blit(d2, X0+246, Y0+230, &(rdpq_blitparms_t){ 
                .theta = angle2, .scale_x = 1-scale2, .scale_y = 1-scale2,
                .cx = 145, .cy = 113,
            });

            rdpq_sprite_blit(d3, X0+266, Y0+256, &(rdpq_blitparms_t){ 
                .theta = -angle3, .scale_x = 1-scale3, .scale_y = 1-scale3,
                .cx = 91, .cy = 24,
            });
        }

        // Draw scrolling logo
        if (anim_part >= 2) {
            rdpq_set_prim_color(white);
            rdpq_sprite_blit(d4, X0 + 161 + (int)scroll4, Y0 + 182, NULL);
        }

        rdpq_detach_show();
    }

    //wait_ms(500); // avoid immediate switch to next screen
    rspq_wait();
    sprite_free(d1);
    sprite_free(d2);
    sprite_free(d3);
    sprite_free(d4);
    //wav64_close(&music);
    display_close();
}

float bg_time = 0;
void render_background(){
    bg_time += display_get_delta_time();
    int alpha = 255 * (0.25f * sinf(bg_time) + 0.25);
    rdpq_set_prim_color(RGBA32(alpha,alpha,alpha,255));
    rdpq_set_env_color(RGBA32(127,127,127,255));
    if(!background_block){
        rspq_block_begin();
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER1((ENV,PRIM,TEX0,TEX0), (0,0,0,TEX0)));
        rdpq_mode_dithering(DITHER_BAYER_INVBAYER);
        rdpq_sprite_blit(background,0,0,NULL);
        background_block = rspq_block_end();
    } rspq_block_run(background_block);

    //rdpq_text_printf(NULL, 3, 50, 50, "^02FPS: %.2f", display_get_fps());
    //heap_stats_t stats; sys_get_heap_stats(&stats);
    //rdpq_text_printf(NULL, 1, 30, 50, "^02MEM: %i total, %i used", stats.total, stats.used);
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER1((ENV,PRIM,TEX0,TEX0), (0,0,0,TEX0)));
    rdpq_mode_dithering(DITHER_BAYER_INVBAYER);
}


void menu_main(){
    if(background_block) {rspq_block_free(background_block); background_block = NULL;}
    if(background) {sprite_free(background); background = NULL;}
    background = sprite_load("rom:/UI/menu.ci8.sprite");
    sprite_t* selector = sprite_load("rom:/UI/effect_pink.sprite");

    //if(bgmusic_name[0] == 0) bgm_play("1_select", true, 0);
    int selection = 0;
    float offset = 400;

    while(true){
        offset = fm_lerp(offset,0, 0.25f);
        joypad_poll();
        joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
        if(joypad_get_axis_pressed(JOYPAD_PORT_1, JOYPAD_AXIS_STICK_Y)){
            sound_play("buttonclick", false);
            selection -= joypad_get_axis_pressed(JOYPAD_PORT_1, JOYPAD_AXIS_STICK_Y);
        }
        selection = iwrap(selection, 0, 3);

        if(pressed.a){
            sound_play("buttonclick", false);
            effects_add_rumble(JOYPAD_PORT_1, 0.1f);
            switch(selection){
                case 0: 
                    coop = false;
                    return;
                    break;
                case 1: 
                    coop = true; 
                    return;
                    break;
                case 2:
                    music_volume(1 - music_volume_get());
                    break;
                case 3: 
                    sound_volume(1 - sound_volume_get());
                    break;
            }
            offset = 400;
        }

        audioutils_mixer_update();
        effects_update();

        rdpq_attach(display_get(), NULL);
        rdpq_set_scissor(0,0, 640, 480);
                if(display_interlace_rdp_field() >= 0) 
             rdpq_enable_interlaced(display_interlace_rdp_field());
        else rdpq_disable_interlaced();
        render_background();

        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,PRIM), (0,0,0,TEX0)));
        rdpq_set_prim_color(RGBA32(100,200,250,255));
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        rdpq_mode_dithering(DITHER_BAYER_INVBAYER);
        rdpq_sprite_blit(selector, 80 + offset, 165 + selection*40, NULL);

        rdpq_textparms_t parmstext = {0}; parmstext.valign = VALIGN_CENTER; parmstext.align = ALIGN_CENTER; parmstext.width = display_get_width() / 2; parmstext.height = 80; parmstext.style_id = 1;
        parmstext.height = 40;
        rdpq_text_printf(&parmstext, 2, 0 + offset,160, maxmap > 0? "Продолжить" : "Играть");
        rdpq_text_printf(&parmstext, 2, 0 + offset,200, "Кооператив");
        rdpq_text_printf(&parmstext, 2, 0 + offset,240, "Музыка: %s", music_volume_get() > 0.5f? "Вкл" : "Выкл");
        rdpq_text_printf(&parmstext, 2, 0 + offset,280, "Звуки: %s", sound_volume_get() > 0.5f? "Вкл" : "Выкл");

        rdpq_detach_show();
    }
    rspq_wait();
    if(background_block) {rspq_block_free(background_block); background_block = NULL;}
    if(background) {sprite_free(background); background = NULL;}

}

typedef struct{
 const char* map_filename;
 float level_fill_max;
 float timer_max;
 const char* music;

 comic_entry_t* comic_pre;
 int comic_entry_pre_count;
} map_t;

map_t maps[] = {
  {.map_filename = "rom:/bar.t3dm", .music = "levela_music", .level_fill_max = 4, .timer_max = 300, .comic_pre = level1_intro, .comic_entry_pre_count = 5},
  {.map_filename = "rom:/bar2.t3dm", .music = "levelb_music", .level_fill_max = 5, .timer_max = 300, .comic_pre = level2_intro, .comic_entry_pre_count = 6},
  {.map_filename = "rom:/bar3.t3dm", .music = "levelc_music", .level_fill_max = 6, .timer_max = 360, .comic_pre = level3_intro, .comic_entry_pre_count = 9},
  {.map_filename = "rom:/bar4.t3dm", .music = "leveld_music", .level_fill_max = 6.5, .timer_max = 360, .comic_pre = level4_intro, .comic_entry_pre_count = 7},
};

map_t* current_map;
float time_left = 0;
bool map_end = false;

typedef struct {
  T3DVec3 position;
} camera_t;
camera_t camera = {0};

float level_fill_current = 0.5f;

typedef enum{
  BOTTLE_HAPPY = 0,
  BOTTLE_SAD = 1,
  BOTTLE_ANGRY = 2,
  BOTTLE_SCARED = 3,
  ITEM_GLASS = 4,
  ITEM_1_FRAME = 5,
  ITEM_2_BEAR = 6,
  ITEM_3_JAWS = 7,
  ITEM_4_BRICK = 8,
  ITEM_5_MEDALLION = 9
} itemtype_t;

typedef struct{
  bool enabled;
  
  uint32_t color;
  itemtype_t itemtype;
  float amount;

  struct{
    float happy, sad, angry, scared;
  } emotions;
  
  const char* modelname;
  T3DModel *model;
  T3DVec3 position_rest;
  T3DVec3 position;
  T3DMat4FP* modelMatFP;

  bool held;

  const char* itemname;

} item_t;

item_t items[] = {
  {.itemtype = BOTTLE_HAPPY,  .enabled = true, .amount = 1.4f, .emotions.happy = 1.0f, .color = 0xc8c864ff, .itemname = "АНТИ-РАДОСТЬ", .modelname = "rom:/bottle_1_happy.t3dm", .position_rest = {{T3D_TOUNITS(1.67153), T3D_TOUNITS(1.36301), T3D_TOUNITS(0.280326)}} },
  {.itemtype = BOTTLE_SAD,    .enabled = true, .amount = 1.4f, .emotions.sad = 1.0f,   .color = 0x6464c8ff, .itemname = "АНТИ-ГРУСТИН", .modelname = "rom:/bottle_2_sad.t3dm", .position_rest = {{T3D_TOUNITS(2.79504), T3D_TOUNITS(1.36301), T3D_TOUNITS(0.280326)}} },
  {.itemtype = BOTTLE_ANGRY,  .enabled = true, .amount = 1.4f, .emotions.angry = 1.0f, .color = 0xc86464ff, .itemname = "ПРОТИВОЗЛОБА", .modelname = "rom:/bottle_3_angry.t3dm", .position_rest = {{T3D_TOUNITS(3.52651 ), T3D_TOUNITS(1.36301), T3D_TOUNITS(0.280326)}} },
  {.itemtype = BOTTLE_SCARED, .enabled = true, .amount = 1.4f, .emotions.scared = 1.0f,.color = 0xc864c8ff, .itemname = "СТРАХА-НЕТ", .modelname = "rom:/bottle_4_scared.t3dm", .position_rest = {{T3D_TOUNITS(4.28118), T3D_TOUNITS(1.36301), T3D_TOUNITS(0.280326)}} },
  {.itemtype = ITEM_GLASS,    .enabled = true, .amount = 1.4f, .itemname = "БОКАЛ",  .modelname = "rom:/glass.t3dm", .position_rest = {{T3D_TOUNITS(6.00018), T3D_TOUNITS(1.19764), T3D_TOUNITS(0.280326)}} },
  {.itemtype = ITEM_1_FRAME,        .enabled = true, .emotions.happy = 0.3f, .emotions.sad = 0.1f,   .itemname = "КАРТИНА", .modelname = "rom:/item1.t3dm", .position_rest = {{T3D_TOUNITS(8.52153), T3D_TOUNITS(1.36301), T3D_TOUNITS(0.280326)}} },
  {.itemtype = ITEM_2_BEAR,         .enabled = true, .emotions.happy = 0.3f, .emotions.scared = 0.2f,  .itemname = "МИШКА", .modelname = "rom:/item2.t3dm", .position_rest = {{T3D_TOUNITS(9.64504), T3D_TOUNITS(1.36301), T3D_TOUNITS(1.370326)}} },
  {.itemtype = ITEM_3_JAWS,         .enabled = true, .emotions.scared = 0.3f, .emotions.angry = 0.1f,  .itemname = "ЧЕЛЮСТИ", .modelname = "rom:/item3.t3dm", .position_rest = {{T3D_TOUNITS(9.64504 ), T3D_TOUNITS(1.36301), T3D_TOUNITS(2.190326)}} },
  {.itemtype = ITEM_4_BRICK,        .enabled = true, .emotions.angry = 0.3f, .emotions.happy = 0.1f,  .itemname = "ОСТРАЯ ЧАСТЬ", .modelname = "rom:/item4.t3dm", .position_rest = {{T3D_TOUNITS(9.64504), T3D_TOUNITS(1.36301), T3D_TOUNITS(3.070326)}} },
  {.itemtype = ITEM_5_MEDALLION,    .enabled = true, .emotions.sad = 0.3f, .emotions.scared = 0.1f,  .itemname = "СТАРЫЙ КУЛОН", .modelname = "rom:/item5.t3dm", .position_rest = {{T3D_TOUNITS(9.64504), T3D_TOUNITS(1.36301), T3D_TOUNITS(3.700326)}} },
};
int itemscount = sizeof(items) / sizeof(item_t);

void items_init(){
  for(int i = 0; i < itemscount; i++){
      item_t* item = &items[i];
      item->model = t3d_model_load(item->modelname);
      item->modelMatFP = malloc_uncached(sizeof(T3DMat4FP));
      item->position = item->position_rest;
      item->enabled = false;
      item->held = false;
  }
}

void items_free(){
  for(int i = 0; i < itemscount; i++){
      item_t* item = &items[i];
      item->model = t3d_model_load(item->modelname);
      item->modelMatFP = malloc_uncached(sizeof(T3DMat4FP));

      if(item->model){ t3d_model_free(item->model); item->model = NULL; }
      if(item->modelMatFP){ free_uncached(item->modelMatFP); item->modelMatFP = NULL; }
      item->position = item->position_rest;
  }
}

void items_update(){
  for(int i = 0; i < itemscount; i++){
      item_t* item = &items[i];
      t3d_mat4fp_from_srt_euler(item->modelMatFP,
          (float[3]){0.1f, 0.1f, 0.1f},
          (float[3]){0.0f, 0, 0},
          item->position.v
      );
  }
}

void items_draw(){
    for(int i = 0; i < itemscount; i++){
      item_t* item = &items[i];
      if(!item->enabled) continue;
      float brightness = (sinf((float)get_ticks_ms() / 200.0f) + 1.0f) * 50.0f; 
      rdpq_set_prim_color(RGBA32(brightness,brightness,brightness, 255));
      t3d_matrix_push(item->modelMatFP);
      t3d_model_draw(item->model);
      t3d_matrix_pop(1);
    }
}


typedef enum{
  STOOL_EMPTY,
  STOOL_COMING,
  STOOL_WAITING_STORY,
  STOOL_WAITING,
  STOOL_WAITING_GLASS,
  STOOL_LEAVING,
} stoolstate_t;

typedef struct {
  stoolstate_t state;

  struct{
    float happy, sad, angry, scared;
  } emotions;

  float amount;

  T3DVec3 position;
  T3DMat4FP* modelMatFP;

  struct {
    bool enabled;
    rspq_block_t *dplDraw;
    T3DModel *model;
    T3DModel *model_waiting;
    T3DModel *model_waiting_timer;
    T3DSkeleton skeleton;
    T3DSkeleton skelBlend;
    T3DMat4FP* modelMatFP;
    // Note that tiny3d internally keeps no track of animations, it's up to the user to manage and play them.
    T3DAnim animIdle;
    T3DAnim animWalk;
    T3DVec3 position;
    float yaw;
    float animblend;
    struct{
      float happy, sad, angry, scared;
      int story[4];
      int order;
    } emotions;

    float timer;

    float result;

    color_t color;

  } person;
  
} stool_t;

stool_t stools[5] = {
  {.position = {{T3D_TOUNITS(1), T3D_TOUNITS(1.2), T3D_TOUNITS(5)}}, .state = STOOL_EMPTY},
  {.position = {{T3D_TOUNITS(3), T3D_TOUNITS(1.2), T3D_TOUNITS(5)}}, .state = STOOL_EMPTY},
  {.position = {{T3D_TOUNITS(5), T3D_TOUNITS(1.2), T3D_TOUNITS(5)}}, .state = STOOL_EMPTY},
  {.position = {{T3D_TOUNITS(7), T3D_TOUNITS(1.2), T3D_TOUNITS(5)}}, .state = STOOL_EMPTY},
  {.position = {{T3D_TOUNITS(9), T3D_TOUNITS(1.2), T3D_TOUNITS(5)}}, .state = STOOL_EMPTY}
}; int stoolscount = 5;


typedef struct {
  T3DVec3 position;
  T3DModel *model;
  T3DSkeleton skeleton;
  T3DSkeleton skelBlend;
  T3DMat4FP* modelMatFP;
  // Note that tiny3d internally keeps no track of animations, it's up to the user to manage and play them.
  T3DAnim animIdle;
  T3DAnim animWalk;
  float inputyaw;
  float animblend;

  int itemheldindex;
  int closeststool;
  rspq_block_t *dplDraw;

  int talking;
  int talking_npc;
  int lives;
} player_t;

player_t players[2];


void stools_init(){
  for(int i = 0; i < stoolscount; i++){
      stool_t* stool = &stools[i];
      stool->modelMatFP = malloc_uncached(sizeof(T3DMat4FP));
      stool->person.modelMatFP = malloc_uncached(sizeof(T3DMat4FP));

      stool->person.model = t3d_model_load("rom:/visitor.t3dm");
      stool->person.model_waiting = t3d_model_load("rom:/waiting.t3dm");
      stool->person.model_waiting_timer = t3d_model_load("rom:/waiting_timer.t3dm");
      stool->person.skeleton = t3d_skeleton_create(stool->person.model);
      stool->person.skelBlend = t3d_skeleton_clone(&stool->person.skeleton, false);
      stool->person.animIdle = t3d_anim_create(stool->person.model, "sitanim");
      stool->person.animWalk = t3d_anim_create(stool->person.model, "walk");
      t3d_anim_attach(&stool->person.animWalk, &stool->person.skelBlend);
      stool->person.yaw = 0;
      t3d_anim_attach(&stool->person.animIdle, &stool->person.skeleton); // tells the animation which skeleton to modify
      stool->person.animblend = 1;
      stool->person.position.x = stool->position.x;
      stool->person.timer = 0;
      stool->person.emotions.order = rand() % 3;
      for(int j = 0; j < 4; j++){
        stool->person.emotions.story[j] = rand() % 3;
      }
  }
}

void stools_free(){
  for(int i = 0; i < stoolscount; i++){
      stool_t* stool = &stools[i];

      { t3d_anim_destroy(&stool->person.animWalk); }
      { t3d_anim_destroy(&stool->person.animIdle); }
      { t3d_skeleton_destroy(&stool->person.skeleton); }
      { t3d_skeleton_destroy(&stool->person.skelBlend); }
      if(stool->person.model){  t3d_model_free(stool->person.model); stool->person.model = NULL;}
      if(stool->person.model_waiting){ t3d_model_free(stool->person.model_waiting); stool->person.model_waiting = NULL;}
      if(stool->person.model_waiting_timer){ t3d_model_free(stool->person.model_waiting_timer);  stool->person.model_waiting_timer = NULL;}
      if(stool->modelMatFP){ free_uncached(stool->modelMatFP); stool->modelMatFP = NULL; }
      if(stool->person.modelMatFP){ free_uncached(stool->person.modelMatFP); stool->person.modelMatFP = NULL; }
      if(stool->person.dplDraw){ rspq_block_free(stool->person.dplDraw); stool->person.dplDraw = NULL; }
      stool->state = STOOL_EMPTY;
  }
}

void  stool_spawn(stool_t* stool){

      stool->person.position.z = T3D_TOUNITS(9);

      stool->person.yaw = 0;
      stool->person.animblend = 1;
      stool->person.position.x = stool->position.x;
      stool->person.timer = 60;
      stool->person.emotions.order = rand() % 3;

      stool->person.emotions.happy    = (float)(rand() % 10) * 0.1f;
      stool->person.emotions.sad      = (float)(rand() % 10) * 0.1f;
      stool->person.emotions.angry    = (float)(rand() % 10) * 0.1f;
      stool->person.emotions.scared   = (float)(rand() % 10) * 0.1f;

      if(mapnumber == 0) {
        stool->person.emotions.angry = 0;
        stool->person.emotions.scared = 0;
      }

      float total = stool->person.emotions.happy + stool->person.emotions.sad + stool->person.emotions.angry + stool->person.emotions.scared;
      if(total == 0) total = 1;
      stool->person.emotions.happy /= total;
      stool->person.emotions.sad /= total;
      stool->person.emotions.angry /= total;
      stool->person.emotions.scared /= total;

      stool->amount = 0;
      stool->emotions.happy = 0;
      stool->emotions.sad = 0;
      stool->emotions.angry = 0;
      stool->emotions.scared = 0;

      for(int j = 0; j < 4; j++){
        stool->person.emotions.story[j] = rand() % 3;
      }
      stool->state = STOOL_COMING;

      stool->person.color = RGBA32(rand() % 255, rand() % 255,rand() % 255,255);
}

void stools_update(){
  for(int i = 0; i < stoolscount; i++){
      stool_t* stool = &stools[i];
      
      t3d_anim_update(&stool->person.animIdle, display_get_delta_time());
      t3d_anim_update(&stool->person.animWalk, display_get_delta_time());

      float previoustimer = stool->person.timer;
      if(stool->state == STOOL_WAITING_GLASS || stool->state == STOOL_WAITING || stool->state == STOOL_WAITING_STORY)
        stool->person.timer -= display_get_delta_time();
      
      if(stool->person.timer < 0 && previoustimer >= 0 && (stool->state == STOOL_WAITING_GLASS || stool->state == STOOL_WAITING || STOOL_WAITING_STORY)){
        stool->state = STOOL_LEAVING;
        debugf("player lives -- \n");
        players[0].lives--;
        sound_play("lifelost", false);
        effects_add_rumble(JOYPAD_PORT_1, 0.25f);
        if(coop) effects_add_rumble(JOYPAD_PORT_2, 0.25f);
      }
        
      
      switch(stool->state){
        case STOOL_COMING:
          {
            if(stool->person.position.z > T3D_TOUNITS(5.5)){
              stool->person.position.z -= T3D_TOUNITS(1) * display_get_delta_time();
              stool->person.yaw = 0;
            }
            else {
              stool->person.animblend = 0;
              stool->state = STOOL_WAITING_STORY;
              sound_play("ding", false);
            }
          } break;

        case STOOL_LEAVING:
          {
            if(stool->person.position.z < T3D_TOUNITS(8)){
              stool->person.position.z += T3D_TOUNITS(1) * display_get_delta_time();
              stool->person.animblend = 1;
              stool->person.yaw = 3.14;
            }
            else {
              stool->state = STOOL_EMPTY;
            }
          } break;
        default: break;
      }
      t3d_skeleton_blend(&stool->person.skeleton, &stool->person.skeleton, &stool->person.skelBlend, stool->person.animblend);
        t3d_mat4fp_from_srt_euler(stool->person.modelMatFP,
          (float[3]){0.1, 0.1, 0.1},
          (float[3]){0.0f, stool->person.yaw, 0},
          stool->person.position.v
        );
        t3d_skeleton_update(&stool->person.skeleton);
  }
}

void stools_draw(){
    item_t* item = &items[ITEM_GLASS];
    for(int i = 0; i < stoolscount; i++){
      stool_t* stool = &stools[i];
      if(stool->state == STOOL_WAITING_GLASS){

        t3d_mat4fp_from_srt_euler(stool->modelMatFP,
          (float[3]){0.1f, 0.1f, 0.1f},
          (float[3]){0.0f, 0, 0},
          stool->position.v
        );

        float brightness = (sinf((float)get_ticks_ms() / 200.0f) + 1.0f) * 50.0f; 
        rdpq_set_prim_color(RGBA32(0,0,0, 255));
        t3d_matrix_push(stool->modelMatFP);
        t3d_model_draw(item->model);
        t3d_matrix_pop(1);
      }

      if(stool->state  != STOOL_EMPTY){
        rdpq_set_env_color(stool->person.color);
        if(!stool->person.dplDraw) {
          rspq_block_begin();
          t3d_matrix_push(stool->person.modelMatFP);
          t3d_model_draw_skinned(stool->person.model, &stool->person.skeleton); // as in the last example, draw skinned with the main skeletont3d_model_draw_skinned(model, &skel); // as in the last example, draw skinned with the main skeletonv
          t3d_matrix_pop(1);
          stool->person.dplDraw = rspq_block_end();
        } rspq_block_run(stool->person.dplDraw);
        if(stool->state == STOOL_WAITING_STORY){
          t3d_matrix_push(stool->person.modelMatFP);
          t3d_model_draw(stool->person.model_waiting);
          t3d_matrix_pop(1);
        }
        if(stool->person.timer < 20){
          t3d_matrix_push(stool->person.modelMatFP);
          t3d_model_draw(stool->person.model_waiting_timer);
          t3d_matrix_pop(1);
        }
      }
    }
}

void player_init(player_t*  player, bool second){
    player->modelMatFP = malloc_uncached(sizeof(T3DMat4FP));
    if(second) player->model = t3d_model_load("rom:/barman_pink.t3dm");
    else player->model = t3d_model_load("rom:/barman_blue.t3dm");
    player->skeleton = t3d_skeleton_create(player->model);
    player->skelBlend = t3d_skeleton_clone(&player->skeleton, false);
    player->animIdle = t3d_anim_create(player->model, "idle");
    player->animWalk = t3d_anim_create(player->model, "walk");
    t3d_anim_attach(&player->animWalk, &player->skelBlend);
    player->position.x = 20;
    player->position.z = 20;
    if(second) player->position.x = 40;
    player->inputyaw = 0;
    t3d_anim_attach(&player->animIdle, &player->skeleton); // tells the animation which skeleton to modify
    player->animblend = 0;
    player->itemheldindex = -1;
    player->closeststool = -1;
    player->talking = -1;
    player->lives = 3;
}

void player_free(player_t*  player, bool second){
    player->position.x = 20;
    player->position.z = 20;
    if(second) player->position.x = 40;
    player->inputyaw = 0;
    player->animblend = 0;
    player->itemheldindex = -1;
    player->closeststool = -1;
    player->talking = -1;
    player->lives = 3;

      { t3d_anim_destroy(&player->animWalk); }
      { t3d_anim_destroy(&player->animIdle); }
      { t3d_skeleton_destroy(&player->skeleton); }
      { t3d_skeleton_destroy(&player->skelBlend); }
      if(player->model){  t3d_model_free(player->model); player->model = NULL;}
      if(player->modelMatFP){ free_uncached(player->modelMatFP); player->modelMatFP = NULL; }
      if(player->dplDraw){ rspq_block_free(player->dplDraw); player->dplDraw = NULL; }
}

void player_update(player_t*  player, bool second){

    //debugf("PLAYER DEBUG 1\n");
    t3d_anim_update(&player->animIdle, display_get_delta_time());

    joypad_inputs_t input = joypad_get_inputs(second? JOYPAD_PORT_2 : JOYPAD_PORT_1);
    joypad_buttons_t pressed = joypad_get_buttons_pressed(second? JOYPAD_PORT_2 : JOYPAD_PORT_1);
    if(player->talking < 0){
      player->position.x += input.stick_x * 0.3 * display_get_delta_time();
      player->position.z -= input.stick_y * 0.3 * display_get_delta_time();
    }

    if(player->position.z < T3D_TOUNITS(1.0)) player->position.z = T3D_TOUNITS(1.0);
    if(player->position.z > T3D_TOUNITS(4.5)) player->position.z = T3D_TOUNITS(4.5);
    if(player->position.x < T3D_TOUNITS(1.0)) player->position.x = T3D_TOUNITS(1.0); 
    if(player->position.x > T3D_TOUNITS(9.0)) player->position.x = T3D_TOUNITS(9.0);  
    //debugf("PLAYER DEBUG 2\n");
    T3DVec3 inputvec = {{-input.stick_x, -input.stick_y, 0.0f}};

    if(player->talking >= 0){
      show_a_sprite = true;
      if(pressed.a){
        int randomtalk = rand() % 3;
        switch(randomtalk){
          case 0: sound_play("talk1", false); break;
          case 1: sound_play("talk2", false); break;
          case 2: sound_play("talk3", false); break;
        }
        player->talking++;
        stools[player->talking_npc].state = STOOL_WAITING;
        debugf("player talking %i", player->talking);
        if(player->talking == 6) {
          player->talking = -1;
          stools[player->talking_npc].state = STOOL_WAITING;
        }
        if(player->talking == 11) {
          player->talking = -1;
          stools[player->talking_npc].state = STOOL_LEAVING;
        }
        if(player->talking == 21) {
          player->talking = -1;
          stools[player->talking_npc].state = STOOL_LEAVING;
        }
        if(player->talking == 31) {
          player->talking = -1;
          stools[player->talking_npc].state = STOOL_LEAVING;
        }
        if(player->talking == 41) {
          player->talking = -1;
          stools[player->talking_npc].state = STOOL_LEAVING;
        }
        if(player->talking == 51) {
          player->talking = -1;
          stools[player->talking_npc].state = STOOL_WAITING;
        }
      }
    }

    if(t3d_vec3_len2(&inputvec) > 0){
      player->animblend = fm_lerp(player->animblend, t3d_vec3_len(&inputvec) / 70, 0.25f);
      t3d_vec3_norm(&inputvec);

      t3d_anim_set_speed(&player->animWalk, player->animblend * 3);
      t3d_anim_update(&player->animWalk, display_get_delta_time());
      t3d_skeleton_blend(&player->skeleton, &player->skeleton, &player->skelBlend, player->animblend);

      float yaw = fm_atan2f(inputvec.x, inputvec.y);
      player->inputyaw = fm_lerp_angle(player->inputyaw, yaw, 0.25f);
    }
    else{
      player->animblend = fm_lerp(player->animblend, 0, 0.25f);
      t3d_skeleton_blend(&player->skeleton, &player->skeleton, &player->skelBlend, player->animblend);
    }
    //debugf("PLAYER DEBUG 3\n");
    {
      float mindist = 99999999; int itemindex = -1;
      if(player->itemheldindex < 0){
        for(int i = 0; i < itemscount; i++){
          item_t* item = &items[i];
          if(!item->enabled) continue;

          float distance = t3d_vec3_distance(&item->position_rest, &player->position);
          if(distance < mindist){
            mindist = distance;
            itemindex = i;
          }
        }
        if(mindist < T3D_TOUNITS(2.0f) && !items[itemindex].held)
          show_a_sprite = true;
        if(input.btn.a && itemindex >= 0 && mindist < T3D_TOUNITS(2.0f) && !items[itemindex].held){
          player->itemheldindex = itemindex;
          sound_play("grab", false);
          items[itemindex].held = true;
          debugf("item %i\n", itemindex);
        }
      }
    }
    //debugf("PLAYER DEBUG 4\n");
     player->closeststool = -1; float mindiststool = 99999999;
     {
        for(int i = 0; i < stoolscount; i++){
          stool_t* stool = &stools[i];
          float distance = t3d_vec3_distance(&stool->position, &player->position);
          if(distance < mindiststool && distance < T3D_TOUNITS(2)){
            mindiststool = distance;
            player->closeststool = i;
          }
        }
    }

    //debugf("PLAYER DEBUG 5\n");
    if(player->itemheldindex >= 0){

      items[player->itemheldindex].position = player->position;
      //int bone_index = t3d_skeleton_find_bone(&player->skeleton, "HoldBone.004.R");
      //if(bone_index >= 0) {
        T3DVec3 offset = {{0,T3D_TOUNITS(1.7f), 0}};
        t3d_vec3_add(&items[player->itemheldindex].position, &items[player->itemheldindex].position, &offset);
        //t3d_vec3_add(&items[player->itemheldindex].position, &items[player->itemheldindex].position, &player->skeleton.bones[bone_index].position);
      //}

      //debugf("PLAYER DEBUG 6\n");
      //if(input.btn.b && t3d_vec3_distance(&items[player->itemheldindex].position_rest, &player->position) < T3D_TOUNITS(2)){
      if(input.btn.b){
        items[player->itemheldindex].position = items[player->itemheldindex].position_rest;
        items[player->itemheldindex].held = false;
        sound_play("put", false);
        player->itemheldindex = -1;
      }
      else{
        if(player->itemheldindex == ITEM_GLASS){
            if(player->closeststool >= 0 && stools[player->closeststool].state == STOOL_WAITING && mindiststool < T3D_TOUNITS(2)){
              show_a_sprite = true;
              if(pressed.a)
                {
                  stools[player->closeststool].state = STOOL_WAITING_GLASS;
                  items[player->itemheldindex].position = items[player->itemheldindex].position_rest;
                  items[player->itemheldindex].held = false;
                  sound_play("put", false);
                  player->itemheldindex = -1;
                }
            }
        }
        else if(player->itemheldindex < ITEM_GLASS){
            if(player->closeststool >= 0 && stools[player->closeststool].state == STOOL_WAITING_GLASS){
              show_a_sprite = true;
              if(input.btn.a && items[player->itemheldindex].amount > 0)
                {
                  debugf("stool emotions = %f %f %f %f\n", stools[player->closeststool].emotions.happy, stools[player->closeststool].emotions.sad, stools[player->closeststool].emotions.angry, stools[player->closeststool].emotions.scared);
                  if(stools[player->closeststool].amount >= 0){
                    show_a_sprite = true;
                    if(pressed.a) sound_play("pour", false);
                    items[player->itemheldindex].amount -= display_get_delta_time() / 2.7f;
                    stools[player->closeststool].emotions.happy += items[player->itemheldindex].emotions.happy * display_get_delta_time();
                    stools[player->closeststool].emotions.sad += items[player->itemheldindex].emotions.sad * display_get_delta_time();
                    stools[player->closeststool].emotions.angry += items[player->itemheldindex].emotions.angry * display_get_delta_time();
                    stools[player->closeststool].emotions.scared += items[player->itemheldindex].emotions.scared * display_get_delta_time();
                    stools[player->closeststool].amount += display_get_delta_time();
                  }
                  if(stools[player->closeststool].amount > 1) {
                        stools[player->closeststool].amount = -1;
                        stools[player->closeststool].emotions.happy = 0;
                        stools[player->closeststool].emotions.sad = 0;
                        stools[player->closeststool].emotions.angry = 0;
                        stools[player->closeststool].emotions.scared = 0;
                        player->talking = 40;
                        player->lives--;
                        sound_play("lifelost", false);
                        effects_add_rumble(JOYPAD_PORT_1, 0.25f);
                        if(coop) effects_add_rumble(JOYPAD_PORT_2, 0.25f);
                        debugf("player lives -- \n");
                  }
                }
            }
        }
        else if(player->itemheldindex > ITEM_GLASS){
            if(player->closeststool >= 0 && stools[player->closeststool].state == STOOL_WAITING_GLASS){
              show_a_sprite = true;
              if(pressed.a)
                {
                  debugf("stool emotions = %f %f %f %f\n", stools[player->closeststool].emotions.happy, stools[player->closeststool].emotions.sad, stools[player->closeststool].emotions.angry, stools[player->closeststool].emotions.scared);
                  if(stools[player->closeststool].amount >= 0){
                    stools[player->closeststool].emotions.happy += items[player->itemheldindex].emotions.happy;
                    stools[player->closeststool].emotions.sad += items[player->itemheldindex].emotions.sad;
                    stools[player->closeststool].emotions.angry += items[player->itemheldindex].emotions.angry;
                    stools[player->closeststool].emotions.scared += items[player->itemheldindex].emotions.scared;
                    stools[player->closeststool].amount += 0.4f;
                    items[player->itemheldindex].position = items[player->itemheldindex].position_rest;
                    items[player->itemheldindex].held = false;
                    items[player->itemheldindex].enabled = false;
                    sound_play("grab", false);
                    player->itemheldindex = -1;
                  }
                  if(stools[player->closeststool].amount > 1) {
                        stools[player->closeststool].amount = -1;
                        stools[player->closeststool].emotions.happy = 0;
                        stools[player->closeststool].emotions.sad = 0;
                        stools[player->closeststool].emotions.angry = 0;
                        stools[player->closeststool].emotions.scared = 0;
                        player->talking = 40;
                        player->lives--;
                        sound_play("lifelost", false);
                        effects_add_rumble(JOYPAD_PORT_1, 0.25f);
                        if(coop) effects_add_rumble(JOYPAD_PORT_2, 0.25f);
                        debugf("player lives -- \n");
                  }
                }
            }
        }
        else if(player->itemheldindex < 0){
            if(player->closeststool >= 0 && stools[player->closeststool].state == STOOL_WAITING_GLASS){
              show_a_sprite = true;
                if(input.btn.a && items[player->itemheldindex].amount > 0)
                  {
                    show_a_sprite = true;
                    if(pressed.a) sound_play("pour", false);
                    debugf("stool emotions = %f %f %f %f\n", stools[player->closeststool].emotions.happy, stools[player->closeststool].emotions.sad, stools[player->closeststool].emotions.angry, stools[player->closeststool].emotions.scared);

                    items[player->itemheldindex].amount -= display_get_delta_time() * 0.6f;
                    stools[player->closeststool].emotions.happy += items[player->itemheldindex].emotions.happy * display_get_delta_time();
                    stools[player->closeststool].emotions.sad += items[player->itemheldindex].emotions.sad * display_get_delta_time();
                    stools[player->closeststool].emotions.angry += items[player->itemheldindex].emotions.angry * display_get_delta_time();
                    stools[player->closeststool].emotions.scared += items[player->itemheldindex].emotions.scared * display_get_delta_time();
                  }
              }
        }

      }

    } else{
      if(player->closeststool >= 0 && stools[player->closeststool].state == STOOL_WAITING_STORY){
        show_a_sprite = true;
          if(pressed.a)
            {
              debugf("stool person emotions = %f %f %f %f\n", stools[player->closeststool].person.emotions.happy, stools[player->closeststool].person.emotions.sad, stools[player->closeststool].person.emotions.angry, stools[player->closeststool].person.emotions.scared);
              player->talking = 0;
              player->talking_npc = player->closeststool;
            }
          }
      if(player->closeststool >= 0 && stools[player->closeststool].state == STOOL_WAITING_GLASS){
        show_a_sprite = true;
          if(pressed.a || pressed.b)
            {
              float filled = stools[player->closeststool].emotions.happy + stools[player->closeststool].emotions.sad + stools[player->closeststool].emotions.angry + stools[player->closeststool].emotions.scared;
              if(player->talking != 50){
                if(filled < 0.35f){
                    player->talking = 10;
                    player->lives--;
                    sound_play("lifelost", false);
                    effects_add_rumble(JOYPAD_PORT_1, 0.25f);
                    if(coop) effects_add_rumble(JOYPAD_PORT_2, 0.25f);
                    debugf("player lives -- \n");
                }
                else{
                  stools[player->closeststool].emotions.happy    /= filled;
                  stools[player->closeststool].emotions.sad      /= filled;
                  stools[player->closeststool].emotions.angry    /= filled;
                  stools[player->closeststool].emotions.scared   /= filled;

                  float distance = fabs((stools[player->closeststool].person.emotions.happy) -  stools[player->closeststool].emotions.happy);
                  distance += fabs((stools[player->closeststool].person.emotions.sad) -  stools[player->closeststool].emotions.sad);
                  distance += fabs((stools[player->closeststool].person.emotions.angry) -  stools[player->closeststool].emotions.angry);
                  distance += fabs((stools[player->closeststool].person.emotions.scared) -  stools[player->closeststool].emotions.scared);

                  debugf("glass finished = %f %f %f %f\n", stools[player->closeststool].person.emotions.happy, stools[player->closeststool].person.emotions.sad, stools[player->closeststool].person.emotions.angry, stools[player->closeststool].person.emotions.scared);
                  debugf("stool = %f %f %f %f\n", stools[player->closeststool].emotions.happy, stools[player->closeststool].emotions.sad, stools[player->closeststool].emotions.angry, stools[player->closeststool].emotions.scared);
                  debugf("distance = %f\n", distance);

                  if(distance < 0.5f){
                    level_fill_current += filled;
                    player->talking = 20;
                    sound_play("correct", false);
                    effects_add_rumble(JOYPAD_PORT_1, 0.25f);
                    if(coop) effects_add_rumble(JOYPAD_PORT_2, 0.25f);
                    
                    if(rand() % 4 == 1) {
                      player->talking = 50;
                      stools[player->closeststool].emotions.happy    = 0;
                      stools[player->closeststool].emotions.sad      = 0;
                      stools[player->closeststool].emotions.angry    = 0;
                      stools[player->closeststool].emotions.scared   = 0;
                      stools[player->closeststool].amount  = 0;
                    }

                  } else{
                    player->talking = 30;
                    sound_play("incorrect", false);
                    effects_add_rumble(JOYPAD_PORT_1, 0.25f);
                    if(coop) effects_add_rumble(JOYPAD_PORT_2, 0.25f);
                  }
                }
              }

              player->talking_npc = player->closeststool;
            }
          }
          
    }
    //debugf("PLAYER DEBUG 9\n");
    t3d_mat4fp_from_srt_euler(player->modelMatFP,
      (float[3]){0.1f, 0.1f, 0.1f},
      (float[3]){0.0f, player->inputyaw, 0},
      player->position.v
    );
    t3d_skeleton_update(&player->skeleton);
    //debugf("PLAYER DEBUG 10\n");
}

void player_draw(player_t* player, bool second){
    if(!player->dplDraw) {
      rspq_block_begin();
      t3d_matrix_push(player->modelMatFP);
      t3d_model_draw_skinned(player->model, &player->skeleton); // as in the last example, draw skinned with the main skeletont3d_model_draw_skinned(model, &skel); // as in the last example, draw skinned with the main skeletonv
      t3d_matrix_pop(1);
      player->dplDraw = rspq_block_end();
    }
    rspq_block_run(player->dplDraw);
}


typedef struct{
  sprite_t* header;
  sprite_t* bottle_blue;
  sprite_t* cup_blue;
  sprite_t* footer_blue;
  sprite_t* effect_blue;

  sprite_t* bottle_pink;
  sprite_t* cup_pink;
  sprite_t* footer_pink;
  sprite_t* effect_pink;
} ui_t;
ui_t ui_elements;

void ui_init(){
  ui_elements.header = sprite_load("rom:/UI/header.sprite");
  ui_elements.bottle_blue = sprite_load("rom:/UI/bottle_blue.sprite");
  ui_elements.cup_blue = sprite_load("rom:/UI/cup_blue.sprite");
  ui_elements.footer_blue = sprite_load("rom:/UI/footer_blue.sprite");
  ui_elements.effect_blue = sprite_load("rom:/UI/effect_blue.sprite");

  ui_elements.bottle_pink = sprite_load("rom:/UI/bottle_pink.sprite");
  ui_elements.cup_pink = sprite_load("rom:/UI/cup_pink.sprite");
  ui_elements.footer_pink = sprite_load("rom:/UI/footer_pink.sprite");
  ui_elements.effect_pink = sprite_load("rom:/UI/effect_pink.sprite");
}

void draw_ui(player_t* player, bool second){
    rdpq_sync_pipe();

    if(player->itemheldindex >= 0 && player->itemheldindex < 4){
          float height_bottom = 345;
          float height = 345 - items[player->itemheldindex].amount * 50;
          rdpq_set_mode_fill(color_from_packed32(items[player->itemheldindex].color));
          if(second)
            rdpq_fill_rectangle(610 - 50, height, 610, height_bottom);
          else rdpq_fill_rectangle(30, height, 80, height_bottom);

      }
    
      if(!second){
        rdpq_set_mode_fill(RGBA32(0,221,211,255));
        float dist = (level_fill_current / current_map->level_fill_max);
        if (dist > 1) dist = 1; 
        rdpq_fill_rectangle(135, 58, 135 + 337.0f * dist, 67);
      }

    if(player->closeststool >= 0 && stools[player->closeststool].state == STOOL_WAITING_GLASS){

      int left = second? 640 - 100 : 56;
      int right = second? 640 - 56 : 100;

      float height_bottom = 415;
      float height = 415 - stools[player->closeststool].emotions.sad * 50;

      rdpq_set_mode_fill(RGBA32(50,50,200,255));
      rdpq_fill_rectangle(left, height, right, height_bottom);
      height_bottom = height;
      height -= stools[player->closeststool].emotions.happy * 50;

      rdpq_set_mode_fill(RGBA32(200,200,100,255));
      rdpq_fill_rectangle(left, height, right, height_bottom);
      height_bottom = height;
      height -= stools[player->closeststool].emotions.angry * 50;

      rdpq_set_mode_fill(RGBA32(200,50,50,255));
      rdpq_fill_rectangle(left, height, right, height_bottom);
      height_bottom = height;
      height -= stools[player->closeststool].emotions.scared * 50;

      rdpq_set_mode_fill(RGBA32(200,100,200,255));
      rdpq_fill_rectangle(left, height, right, height_bottom);
    }
    rdpq_set_mode_copy(true);
    if(!second){
      rdpq_sprite_blit(ui_elements.header, 70,30, NULL);
    }

    if(player->itemheldindex >= 0 && player->itemheldindex < 4) rdpq_sprite_blit(second? ui_elements.bottle_pink : ui_elements.bottle_blue, second? 610 - 65 : 30,210, NULL);
    if(player->closeststool >= 0 && stools[player->closeststool].state == STOOL_WAITING_GLASS) rdpq_sprite_blit(second? ui_elements.cup_pink : ui_elements.cup_blue, second? 610 - 80 : 30,350, NULL);
    rdpq_sprite_blit(second? ui_elements.footer_pink : ui_elements.footer_blue, second? 610 - 180 : 30,425, NULL);
    if(player->itemheldindex >= 0) rdpq_sprite_blit(second? ui_elements.effect_pink : ui_elements.effect_blue, second? 610 - 230 : 110, 410, NULL); 

    if(!second){
      rdpq_set_mode_fill(RGBA32(0,221,211,255));
      for(int i = 0; i < players[0].lives; i++){
        rdpq_fill_rectangle(42 + 12*i, 435, 50 + 12*i, 445);
      }
    } else {
      rdpq_set_mode_fill(RGBA32(221,0,211,255));
      for(int i = 0; i < players[0].lives; i++){
        rdpq_fill_rectangle(610 - 20 - 12*i, 435, 610 - 12 - 12*i, 445);
      }
    }

    if(player->itemheldindex >= 0){
      rdpq_text_printf(NULL, 2, second? 610 - 200 : 130, 430, "%s", items[player->itemheldindex].itemname); 
    }
    if(!second){
      rdpq_text_printf(NULL, 2, 164, 50, "СБОР ЧЕЛОВЕЧЕСКИХ ДАННЫХ"); 
      rdpq_text_printf(NULL, 3, 520, 50, "%02i:%02i", (int)time_left / 60, (int)time_left % 60);
    }

    if(player->talking >= 0){
      const char* text = NULL;
      int style = 0;
      if(player->talking == 0){
        text = "Как же хочется уйти от всех этих эмоций!";
      }
      else if(player->talking == 5){
        text = "Смешай мне чего-нибудь подходящего!";
      }
      else if (player->talking < 5){
        int strongness = 0;
        int order = stools[player->talking_npc].person.emotions.order;
        int curemotion = 0;
        switch(order){
          case 0:
            curemotion = player->talking - 1;
            break;
          case 1:
            curemotion = 4 - player->talking;
            break;
          default:
            if(player->talking == 1) curemotion = 2;
            if(player->talking == 2) curemotion = 1;
            if(player->talking == 3) curemotion = 3;
            if(player->talking == 4) curemotion = 0;
            break;
        }

        switch(curemotion){
          case 0:
            text = backstory_list_happy[(int)(stools[player->talking_npc].person.emotions.happy * 9.0f)][stools[player->talking_npc].person.emotions.story[curemotion]];
            break;
          case 1:
            text = backstory_list_sad[(int)(stools[player->talking_npc].person.emotions.sad * 9.0f)][stools[player->talking_npc].person.emotions.story[curemotion]];
            break;
          case 2:
            text = backstory_list_angry[(int)(stools[player->talking_npc].person.emotions.angry * 9.0f)][stools[player->talking_npc].person.emotions.story[curemotion]];
            break;
          default:
            text = backstory_list_scared[(int)(stools[player->talking_npc].person.emotions.scared * 9.0f)][stools[player->talking_npc].person.emotions.story[curemotion]];
            break;
        }
        style = curemotion + 1;
      }
      else{
        if(player->talking == 10){
          text = "Вы закончили? Но тут же ничего нет! Я тогда пойду в другой бар!";
        }
        if(player->talking == 30){
          text = "Нет, эта смесь не помогает! Меня переполняют эмоции! ААРХ! Я не знаю что мне делать!";
        }
        if(player->talking == 20){
          text = "Да, то что надо! Я вообще ничего не чувствую! Спасибо!";
        }
        if(player->talking == 40){
          text = "Аа! Ты пролил на меня весь бокал! Я больше сюда никогда не вернусь!";
        }
        if(player->talking == 50){
          text = "Да, то что надо! Еще! Налей мне еще того же!";
        }
      }
        rdpq_textparms_t textparms = {0};
        textparms.width = 300;
        textparms.height = 400;
        textparms.align = ALIGN_LEFT; textparms.valign = VALIGN_TOP;
        textparms.wrap = WRAP_WORD;
        textparms.style_id = style;
      rdpq_text_printf(&textparms, 3, second? 330 : 30, 90, "КЛИЕНТ:\n%s", text);
    }

    if(!second)
    if(map_end){
      if(players[0].lives <= 0){
        rdpq_text_printf(NULL, 2, 240, 240, "ЖИЗНИ КОНЧИЛИСЬ!\nИГРА ОКОНЧЕНА!");
      }
      else if (time_left < 0) {
        rdpq_text_printf(NULL, 2, 240, 240, "ВРЕМЯ КОНЧИЛОСЬ!\nИГРА ОКОНЧЕНА!");
      }
      else if(level_fill_current >= current_map->level_fill_max){
        rdpq_text_printf(NULL, 2, 240, 240, "УРОВЕНЬ ПРОЙДЕН!");
      }
    }
}

/**
 * Simple example with a 3d-model file created in blender.
 * This uses the builtin model format for loading and drawing a model.
 */

void init(){
  debug_init_isviewer();
	debug_init_usblog();
	asset_init_compression(2);
  //wav64_init_compression(3);

  dfs_init(DFS_DEFAULT_LOCATION);
  joypad_init();

  vi_init();
  rdpq_init();

  audio_init(28000, 8);
  mixer_init(24); 

  audio_prewarm_all();
}

void setup(){

  display_init((resolution_t){.width = 640, .height = 480, .interlaced = INTERLACE_RDP}, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE_ANTIALIAS_DEDITHER);
  if(get_tv_type() == TV_PAL) {
      vi_set_borders((vi_borders_t){.up = 48, .down = 48});
      vi_set_yscale_factor(2.0f);
  }
  t3d_init((T3DInitParams){});

  ui_init();

  font = rdpq_font_load("rom:/BulatovSPDemo.font64");
  font2 = rdpq_font_load("rom:/TTHoves-Medium.font64");

  rdpq_text_register_font(2, font);
  rdpq_text_register_font(3, font2);

  rdpq_font_style(font, 0, &(rdpq_fontstyle_t){.color = RGBA32(20,14,60,255), .outline_color = RGBA32(189,185,222,255)});
  rdpq_font_style(font, 1, &(rdpq_fontstyle_t){.color = RGBA32(90,200,250,255), .outline_color = RGBA32(40,100,150,255)});

  rdpq_font_style(font2, 1, &(rdpq_fontstyle_t){.color = RGBA32(250,250,50,255), .outline_color = RGBA32(50,50,25,255)});
  rdpq_font_style(font2, 2, &(rdpq_fontstyle_t){.color = RGBA32(28,170,230,255), .outline_color = RGBA32(25,25,50,255)});
  rdpq_font_style(font2, 3, &(rdpq_fontstyle_t){.color = RGBA32(250,50,50,255), .outline_color = RGBA32(50,25,25,255)});
  rdpq_font_style(font2, 4, &(rdpq_fontstyle_t){.color = RGBA32(250,50,250,255), .outline_color = RGBA32(50,25,50,255)});

  rdpq_font_style(font2, 5, &(rdpq_fontstyle_t){.color = RGBA32(28,170,230,255), .outline_color = RGBA32(25,50,75,255)});
  rdpq_font_style(font2, 6, &(rdpq_fontstyle_t){.color = RGBA32(250,50,250,255), .outline_color = RGBA32(50,25,50,255)});

  rdpq_font_style(font2, 0, &(rdpq_fontstyle_t){.outline_color = RGBA32(20,14,60,255), .color = RGBA32(189,185,222,255)});

  rdpq_text_register_font(FONT_BUILTIN_DEBUG_MONO, rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_MONO));

  srand(getentropy32());
  register_VI_handler((void(*)())rand);

  effects_rumble_stop();
}

void map_init(int mapnum){
  exposure = 5;
  switch(mapnum){
    case 0:
      items[0].enabled = true;
      items[0].amount = 1.4f;
      items[1].enabled = true;
      items[1].amount = 1.4f;
      items[4].enabled = true;
      break;
    case 1:
      items[0].enabled = true;
      items[0].amount = 1.3f;
      items[1].enabled = true;
      items[1].amount = 1.3f;
      items[2].enabled = true;
      items[2].amount = 1.3f;
      items[3].enabled = true;
      items[3].amount = 1.3f;
      items[4].enabled = true;
      break;
    case 2:
      items[0].enabled = true;
      items[0].amount = 0.85f;
      items[1].enabled = true;
      items[1].amount = 0.85f;
      items[2].enabled = true;
      items[2].amount = 0.85f;
      items[3].enabled = true;
      items[3].amount = 0.85f;
      items[4].enabled = true;
      items[5].enabled = true;
      items[6].enabled = true;
      items[7].enabled = true;
      items[8].enabled = true;
      items[9].enabled = true;
      break;
    case 3:
      items[0].enabled = true;
      items[0].amount = 0.4f;
      items[1].enabled = true;
      items[1].amount = 0.4f;
      items[2].enabled = true;
      items[2].amount = 0.4f;
      items[3].enabled = true;
      items[3].amount = 0.4f;
      items[4].enabled = true;
      items[5].enabled = true;
      items[6].enabled = true;
      items[7].enabled = true;
      items[8].enabled = true;
      items[9].enabled = true;
      break;
  }
}

sprite_t* a_button;

int main()
{
  init();
  effects_rumble_stop();
  libdragon_logo();
  setup();
  bgm_hardplay("menu_music", true, 0.1f);
  
  show_comic(logo_intro, 1);

  rspq_wait();
  rspq_wait();
    rspq_wait();
      rspq_wait();
        rspq_wait();
          rspq_wait();

  a_button = sprite_load("rom:/UI/button_a.rgba32.sprite");

  while(true){
    menu_main();
    if(maxmap == 0){
      bgm_hardplay("intro_music", true, 0.1f);
      show_comic(comics_intro, 7);
    }

    bool infinitemode = false;

    for(mapnumber = maxmap; mapnumber < 4; mapnumber++){
    map_start:

      if(mapnumber > maxmap) maxmap = mapnumber;
      if(maxmap > 3) maxmap = 3;

      current_map = &maps[mapnumber];

      if(mapnumber > 0) bgm_hardplay("dialogue_music", true, 0.1f);

      show_comic(maps[mapnumber].comic_pre, maps[mapnumber].comic_entry_pre_count);
      //show_comic(comics_intro, comics_intro_texts, 12);

      T3DViewport viewport = t3d_viewport_create();

      T3DMat4 modelMat; // matrix for our model, this is a "normal" float matrix
      t3d_mat4_identity(&modelMat);

      bgm_hardplay(maps[mapnumber].music, true, 0.1f);
      // Now allocate a fixed-point matrix, this is what t3d uses internally.
      // Note: this gets DMA'd to the RSP, so it needs to be uncached.
      // If you can't allocate uncached memory, remember to flush the cache after writing to it instead.
      T3DMat4FP* modelMatFP = malloc_uncached(sizeof(T3DMat4FP));
      T3DMat4FP* modelblockerMatFP = malloc_uncached(sizeof(T3DMat4FP));

      T3DVec3 camPos = {0};
      T3DVec3 camTarget = {0};

      uint8_t colorAmbient[4] = {255, 255, 255, 0xFF};
      uint8_t colorAmbientB[4] = {20, 20, 70, 0xFF};
      uint8_t colorDir[4]     = {0xFF, 0xFF, 0xFF, 0xFF};

      T3DVec3 lightDirVec = {{-1.0f, 1.0f, 1.0f}};
      t3d_vec3_norm(&lightDirVec);

      time_left = current_map->timer_max;

      // Load a model-file, this contains the geometry and some metadata
      T3DModel *scene = t3d_model_load(current_map->map_filename);
      player_init(&players[0], false);
      if(coop) player_init(&players[1], true);
      items_init();
      stools_init();
      map_init(mapnumber);

      float rotAngle = 0.0f;
      rspq_block_t *dplDraw = NULL;

      float nextperson_time = 0;
      float nextitem_time = 0;
      float timer_end = 5.0f;
      map_end = false;
      int framecount = 0;

      bool paused = false;

      for(;;)
      {
        show_a_sprite = false;
        framecount++;
        // ======== Update ======== //
        rotAngle -= 0.00f;
        float modelScale = 0.1f;
        exposure = fm_lerp(exposure, 1, 0.05f);
        audioutils_mixer_update();
        effects_update();

        joypad_poll();

        joypad_buttons_t held = joypad_get_buttons_held(JOYPAD_PORT_1);
        joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);

        if(infinitemode){
          players[0].lives = 3;
          level_fill_current = 0;
          items[0].amount = 1.0f;
          items[1].amount = 1.0f;
          items[2].amount = 1.0f;
          items[3].amount = 1.0f;
          time_left = 300;
        }

        if(held.l && held.start){
          level_fill_current = current_map->level_fill_max;
        }

        if(held.z && held.start){
          players[0].lives = 0;
        }

        if(pressed.start){
          paused = !paused;
        }

        if(paused && pressed.b){
          mapnumber = 10;
          goto goto_map_end;
        }
        if(paused && pressed.a){
          players[0].lives = 0;
          paused = false;
        }
        if(!paused){
            if(players[0].lives > 0 && level_fill_current < current_map->level_fill_max && time_left > 0){

            if(nextperson_time <= 0){
              if(coop) nextperson_time = (rand() % 11) + 11;
              else nextperson_time = (rand() % 16) + 15;
              bool found =  false;
              for(int i = 0; i < stoolscount && !found; i++){
                if(stools[i].state == STOOL_EMPTY){
                  stool_spawn(&stools[i]);
                  found = true;
                }
              }
            } else{
              nextperson_time -= display_get_delta_time();
            }

            if(mapnumber >= 2){
              if(nextitem_time <= 0){
                if(coop) nextitem_time = (rand() % 3) + 3;
                else nextitem_time = (rand() % 6) + 5;
                bool found =  false;
                for(int i = 0; i < 5 && !found; i++){
                  if(!items[i+5].enabled){
                    items[i+5].enabled = true;
                    found = true;
                  }
                }
              } else{
                nextitem_time -= display_get_delta_time();
              }
            }

            player_update(&players[0], false);
            if(coop) player_update(&players[1], true);
            items_update();
            stools_update();

            time_left -= display_get_delta_time();
          } else {
            map_end = true;
            timer_end -= display_get_delta_time();
            if(timer_end < 0)
              goto goto_map_end;
          }
        }

        if(infinitemode){
          players[0].lives = 3;
          level_fill_current = 0;
          items[0].amount = 1.0f;
          items[1].amount = 1.0f;
          items[2].amount = 1.0f;
          items[3].amount = 1.0f;
          time_left = 300;
        }

        camera.position = players[0].position;
        if(coop){
          T3DVec3 campcoop;
          t3d_vec3_add(&campcoop, &players[0].position, &players[1].position);
          t3d_vec3_scale(&campcoop, &campcoop, 0.5f);
          camera.position  = campcoop;
        }
        
        t3d_vec3_lerp(&camTarget, &camTarget, &camera.position, 0.1f);
        const T3DVec3 camOffset = {{-5,25,32}};
        t3d_vec3_add(&camPos, &camTarget, &camOffset);

        t3d_viewport_set_projection(&viewport, T3D_DEG_TO_RAD(65.0f), 5.0f, 80.0f);
        t3d_viewport_look_at(&viewport, &camPos, &camTarget, &(T3DVec3){{0,1,0}});

        // slowly rotate model, for more information on matrices and how to draw objects
        // see the example: "03_objects"
        t3d_mat4_from_srt_euler(&modelMat,
          (float[3]){modelScale, modelScale, modelScale},
          (float[3]){0.0f, -rotAngle * 0.5f, 0},
          (float[3]){0,0,0}
        );
        t3d_mat4_to_fixed(modelMatFP, &modelMat);

        t3d_mat4fp_from_srt_euler(modelblockerMatFP,
          (float[3]){modelScale, modelScale, modelScale},
          (float[3]){0.0f, rotAngle * 0.5f, 0},
          (float[3]){0,0,0}
        );

        audioutils_mixer_update();

        // ======== Draw ======== //
        rdpq_attach(display_get(), display_get_zbuf());
        t3d_frame_start();
        t3d_viewport_attach(&viewport);
        rdpq_set_scissor(0,0, 640, 480);
        if(display_interlace_rdp_field() >= 0) 
             rdpq_enable_interlaced(display_interlace_rdp_field());
        else rdpq_disable_interlaced();
        if(framecount % 15) t3d_screen_clear_color(RGBA32(0,0,0,0));
        rdpq_set_scissor(30,30, 610, 450);

        //t3d_screen_clear_color(RGBA32(100, 80, 80, 0xFF));
        t3d_screen_clear_depth();

        t3d_light_set_ambient(colorAmbient);
        t3d_light_set_directional(0, colorDir, &lightDirVec);
        t3d_light_set_count(0);
        t3d_light_set_exposure(exposure);

        rdpq_mode_zbuf(false,false);
        rdpq_mode_antialias(AA_REDUCED);

        // you can use the regular rdpq_* functions with t3d.
        // In this example, the colored-band in the 3d-model is using the prim-color,
        // even though the model is recorded, you change it here dynamically.
        
        t3d_matrix_push(modelMatFP);
        if(!dplDraw) {
          rspq_block_begin();

          // Draw the model, material settings (e.g. textures, color-combiner) are handled internally
          T3DModelState matstate = t3d_model_state_create();
          for(int i = 0; i < bar_matqueue_a_count; i++){
            T3DModelIter iter = t3d_model_iter_create(scene, T3D_CHUNK_TYPE_OBJECT);
            while(t3d_model_iter_next(&iter)){
              if(!strcmp(iter.object->material->name, bar_matqueue_a[i])){
                t3d_model_draw_material(iter.object->material, &matstate);
                t3d_model_draw_object(iter.object, NULL);
              }
            }
          }

          dplDraw = rspq_block_end();
        }
        // for the actual draw, you can use the generic rspq-api.
        rspq_block_run(dplDraw);
        t3d_matrix_pop(1);

        rdpq_mode_zbuf(true,true);
        t3d_light_set_ambient(colorAmbientB);
        t3d_light_set_count(1);
        items_draw();
        player_draw(&players[0], false);
        if(coop) player_draw(&players[1], true);

        rdpq_mode_zbuf(false,false);
        t3d_matrix_push(modelMatFP);

        t3d_light_set_ambient(colorAmbient);
        t3d_light_set_count(0);
        {
          T3DModelState matstate = t3d_model_state_create();
          for(int i = 0; i < bar_matqueue_b_count; i++){
            T3DModelIter iter = t3d_model_iter_create(scene, T3D_CHUNK_TYPE_OBJECT);
            while(t3d_model_iter_next(&iter)){
              if(!strcmp(iter.object->material->name, bar_matqueue_b[i])){
                t3d_model_draw_material(iter.object->material, &matstate);
                t3d_model_draw_object(iter.object, NULL);
              }
            }
          }
        }
        t3d_matrix_pop(1);

        stools_draw();

        rdpq_sync_pipe();
        rdpq_sync_tile();

        draw_ui(&players[0], false);
        if(coop) draw_ui(&players[1], true);

        if(paused){
          rdpq_text_printf(NULL, 2, 200,200, "На паузе\n\nСТАРТ - продолжить\nA - Переиграть\nB - Выход в меню");
        }

        if(show_a_sprite){
          rdpq_sync_pipe();
          rdpq_sync_tile();
          rdpq_set_mode_standard();
          rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
          rdpq_sprite_blit(a_button, 300, 400, NULL);
        }

        audioutils_mixer_update();

        //rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, 20, 20, "player %f %f %f - stool 0 %f %f %f", player->position.x, player->position.y, player->position.z, stools[0].position.x, stools[0].position.y, stools[0].position.z);
        //rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, 20, 20, "player talking %i", player[0].talking);
        //rdpq_text_printf(NULL, FONT_BUILTIN_DEBUG_MONO, 20, 20, "fps %f", player[0].talking);
        rdpq_detach_show();
      }

      goto_map_end:

      if(players[0].lives <= 0 || time_left <= 0) mapnumber--;

      rspq_wait();

      free_uncached(modelMatFP);
      free_uncached(modelblockerMatFP);
      t3d_model_free(scene);
      rspq_block_free(dplDraw);

      player_free(&players[0], false);
      if(coop) player_free(&players[1], true);
      items_free();
      stools_free();
      level_fill_current = 0.5f;
      

    }

    if(mapnumber == 4){
      bgm_hardplay("outro_music", true, 0.1f);
      show_comic(outro_comic, 3);
      show_comic_credits(credits_comic, 6);
      if(enable_infinite_mode()){
        infinitemode = true;
        mapnumber = 3;
        goto map_start;
      }
    }
  }

  t3d_destroy();
  return 0;
}
