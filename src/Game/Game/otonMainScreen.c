#include "otonMainScreen.h"

#include "Graphics/graphics.h"
#include "Graphics/images.h"
#include "Graphics/spineGfx.h"
#include "Graphics/camera.h"
#include "Graphics/debugRendering.h"
#include "Graphics/imageSheets.h"
#include "Input/input.h"
#include "System/platformLog.h"
#include "collisionDetection.h"
#include "Graphics/sprites.h"
#include "Utils/helpers.h"
#include "tween.h"
#include "System/random.h"
#include "Utils/stretchyBuffer.h"
#include "UI/text.h"
#include "Utils/cfgFile.h"
#include "sound.h"

static float secondTimer;
static float timeScale;
static int secondCount;

static int renderWidth, renderHeight;

static int bg0Img = -1;
static int bg1Img = -1;
static int bg2Img = -1;

static EntityID bg0;
static EntityID bg1;
static EntityID bg2;

static Vector2 bgBasePos;
static Vector2 bgOffsetPos;

static float timeUntilBreeze;

static Tween breezeTween;

static void loadBG( void )
{
	bg0Img = img_Load( "Images/bg_0.png", ST_DEFAULT );
	bg1Img = img_Load( "Images/bg_1.png", ST_DEFAULT );
	bg2Img = img_Load( "Images/bg_2.png", ST_DEFAULT );

	bgBasePos = vec2( renderWidth / 2.0f, renderHeight / 2.0f );
	bgOffsetPos = bgBasePos;
	bgOffsetPos.x -= 10.0f;

	bg0 = spr_CreateSprite( bg0Img, 1, bgBasePos, VEC2_ONE, 0.0f, CLR_WHITE, -50 );
	bg1 = spr_CreateSprite( bg1Img, 1, bgBasePos, VEC2_ONE, 0.0f, CLR_WHITE, -51 );
	bg2 = spr_CreateSprite( bg2Img, 1, bgBasePos, VEC2_ONE, 0.0f, CLR_WHITE, -52 );

#if 0
	timeUntilBreeze = rand_GetRangeFloat( NULL, 10.0f, 45.0f );

	setTween( &breezeTween, 0.0f, 1.0f, rand_GetRangeFloat( NULL, 1.25f, 2.25f ), NULL );
#endif
}

static void physicsTick_BG( float dt )
{
#if 0
	float scaledDT = dt * timeScale;
	timeUntilBreeze -= scaledDT;
	if( timeUntilBreeze <= 0.0f ) {
		timeUntilBreeze = rand_GetRangeFloat( NULL, 10.0f, 45.0f );
		setTween( &breezeTween, 0.0f, 1.0f, rand_GetRangeFloat( NULL, 1.25f, 2.25f ), easeInOutQuad );
	}

	processTween( &breezeTween, scaledDT );
	float curr = breezeTween.current;
	if( curr > 0.5f ) {
		curr = 1.0f - curr;
	}
	curr *= 2.0f;
	Vector2 newPos;
	vec2_Lerp( &bgBasePos, &bgOffsetPos, curr, &newPos );
	spr_UpdatePos( bg0, &newPos );
	spr_UpdatePos( bg2, &newPos );
#endif
}

static int explosionSnd[] = { -1 };

static int hitByEnemySnd[] = { -1 };
static int hitEnemySnds[] = { -1, -1, -1, -1 };
static int shootSnds[] = { -1, -1, -1 };
static int slashSnds[] = { -1, -1, -1, -1 };

static int music = -1;
static int ambience = -1;

static void loadSounds( void )
{
	shootSnds[0] = snd_LoadSample( "Sounds/shoot_d.ogg", 1, false );
	shootSnds[1] = snd_LoadSample( "Sounds/shoot_f.ogg", 1, false );
	shootSnds[2] = snd_LoadSample( "Sounds/shoot_gs.ogg", 1, false );

	slashSnds[0] = snd_LoadSample( "Sounds/attack_b.ogg", 1, false );
	slashSnds[1] = snd_LoadSample( "Sounds/attack_c.ogg", 1, false );
	slashSnds[2] = snd_LoadSample( "Sounds/attack_ds.ogg", 1, false );
	slashSnds[3] = snd_LoadSample( "Sounds/attack_g.ogg", 1, false );

	hitEnemySnds[0] = snd_LoadSample( "Sounds/hit_b.ogg", 1, false );
	hitEnemySnds[1] = snd_LoadSample( "Sounds/hit_c.ogg", 1, false );
	hitEnemySnds[2] = snd_LoadSample( "Sounds/hit_ds.ogg", 1, false );
	hitEnemySnds[3] = snd_LoadSample( "Sounds/hit_g.ogg", 1, false );

	hitByEnemySnd[0] = snd_LoadSample( "Sounds/death.ogg", 1, false );

	explosionSnd[0] = snd_LoadSample( "Sounds/explosion.ogg", 1, false );

	music = snd_LoadStreaming( "Music/spooky.ogg", true, 1 );
	snd_PlayStreaming( music, 0.25f, 0.0f, 0 );

	ambience = snd_LoadStreaming( "Music/ambience.ogg", true, 1 );
	snd_PlayStreaming( ambience, 0.1f, 0.0f, 0 );
}

static void playHitEnemy( void )
{
	static int lastPlayed = -1;
	int play;
	do {
		play = rand_GetArrayEntry( NULL, ARRAY_SIZE( hitEnemySnds ) );
	} while( play == lastPlayed );
	snd_Play( hitEnemySnds[play], 0.25f, 1.0f, 0.0f, 0 );
	lastPlayed = play;
}

static void playHitByEnemy( void )
{
	snd_Play( hitByEnemySnd[0], 0.25f, 1.0f, 0.0f, 0 );
}

static void playExplosion( float normalizedPower )
{
	snd_Play( explosionSnd[0], 0.25f * normalizedPower, rand_GetRangeFloat( NULL, 0.9f, 1.1f ), 0.0f, 0 );
}

static void playShoot( void )
{
	static int lastPlayed = -1;
	int play;
	do {
		play = rand_GetArrayEntry( NULL, ARRAY_SIZE( shootSnds ) );
	} while( play == lastPlayed );
	snd_Play( shootSnds[play], 0.25f, 1.0f, 0.0f, 0 );
	snd_Play( shootSnds[play], 0.25f, 1.0f, 0.0f, 0 );
	lastPlayed = play;
}

static void playAttack( void )
{
	static int lastPlayed = -1;
	int play;
	do {
		play = rand_GetArrayEntry( NULL, ARRAY_SIZE( slashSnds ) );
	} while( play == lastPlayed );
	snd_Play( slashSnds[play], 0.25f, 1.0f, 0.0f, 0 );
	lastPlayed = play;
}

static int score;
static int highScore;

static int displayFnt = -1;
static int infoFnt = -1;

static int whiteImg = -1;
static int explosionImg = -1;

static float pauseTimeLeft;
static bool forceTimeScale;

static Collider bottomArenaCollider;
static Collider leftArenaCollider;
static Collider rightArenaCollider;
static Collider topArenaCollider;

static int totalSpawnScore;
static int maxSpawnScore;

typedef struct {
	EntityID sprite;
	Vector2 velocity;
	float rotVelocity;
	
	Tween alphaTween;
	Tween scaleTween;
} Particle;

typedef enum {
	CS_IDLE,
	CS_SLASH_1,
	CS_SLASH_2,
	CS_JUMP_UP,
	CS_AIR_IDLE,
	CS_FALL,
	CS_LAND,
	CS_AIR_SLASH_1,
	CS_AIR_SLASH_2,
	CS_FALL_ATTACK,
	CS_DEATH,
	NUM_CHARACTER_STATES
} CharacterState;

typedef struct {
	Vector2 pos;
	Vector2 velocity;

	Collider bodyCollider;
	Collider weaponCollider;
	Collider damageCollider;

	EntityID sprite;

	CharacterState state;
	float facing;

	bool slashOne;

	float heightAtAttackStart;

	bool canAirJump;

	float timeInState;
} PlayerCharacter;

static PlayerCharacter player;

static void pauseTime( float duration )
{
	timeScale = 0.0f;
	pauseTimeLeft = duration;
}

typedef enum {
	ET_INVALID = -1,
	ET_ZOMBIE,
	ET_PENANGGALAN,
	ET_JUMPER,
	ET_SHOOTER,
	ET_PROJECTILE,
	NUM_ENEMY_TYPES
} EnemyType;

typedef struct {
	EnemyType type;
	Vector2 pos;
	Vector2 velocity;
	Collider collision;
	EntityID sprite;

	bool isAlive;
	float timeAlive;
	float facing;
	bool snapNewPos;
	float speed;
	int state;
	float otherTimer;
	Vector2 targetPos;

	bool shouldDestroy;
} Enemy;

typedef struct {
	void ( *create )( void );
	const char* aliveImgPath;
	const char* deadImgPath;

	int aliveImg;
	int deadImg;

	void ( *update )( Enemy* );

	int typeScore;
} EnemyTypeData;

static Enemy* createBaseEnemy( EnemyType type, Vector2 pos, float facing );
static void createZombie( void );
static void createPenanggalan( void );
static void createJumper( void );
static void createShooter( void );

static Collider enemyColliderTypes[NUM_ENEMY_TYPES];

static float enemyDT;

static Enemy* sbEnemies = NULL;

ColliderCollection enemyColliderCollection = { NULL, sizeof( Enemy ), 0 };

static void deadEnemyUpdate( Enemy* enemy )
{
	Color clr = CLR_WHITE;
	clr.a = clamp01( 1.0f - ( enemy->timeAlive / 0.5f ) );
	spr_UpdateColor( enemy->sprite, &clr );
	if( enemy->timeAlive >= 0.5f ) {
		// destroy and spawn particles
		enemy->shouldDestroy = true;
	}
}

static void wrapEnemy( Enemy* enemy )
{
	// if we're off the edge of the map then loop around
	if( ( enemy->facing < 0 ) && ( enemy->pos.x < -32.0f ) ) {
		enemy->pos.x = renderWidth + 32.0f;
		enemy->snapNewPos = true;
	} else if( ( enemy->facing > 0 ) && ( enemy->pos.x > ( renderWidth + 32.0f ) ) ) {
		enemy->pos.x = -32.0f;
		enemy->snapNewPos = true;
	}
}

static void zombieUpdate( Enemy* enemy )
{
	wrapEnemy( enemy );
}

static void penanggalanUpdate( Enemy* enemy )
{
	wrapEnemy( enemy );

	// update y position using a sinusodial function based on time alive
	enemy->velocity.y = SDL_sinf( enemy->timeAlive ) * 100.0f;
}

static void jumperUpdate( Enemy* enemy )
{
	wrapEnemy( enemy );

	// stay still for 1 second, they jump for one second
	if( enemy->state == 1 ) {
		enemy->velocity.y += 1000.0f * enemyDT;
	}

	if( enemy->state == 0 ) {
		if( enemy->timeAlive >= 1.0f ) {
			enemy->state = 1;
			enemy->timeAlive = 0.0f;
			// jump
			enemy->velocity.x = enemy->speed * enemy->facing;
			enemy->velocity.y = -400.0f;
		}
	}

	if( enemy->pos.y > renderHeight - 96.0f ) {
		enemy->state = 0;
		enemy->velocity.x = 0.0f;
		enemy->velocity.y = 0.0f;
		enemy->pos.y = renderHeight - 96.0f;
	}
}

static void shooterChooseTarget( Enemy* enemy )
{
	enemy->targetPos = vec2( rand_GetRangeFloat( NULL, 40.0f, renderWidth - 40.0f ), rand_GetRangeFloat( NULL, 50.0f, 150.0f ) );
	enemy->facing = sign( enemy->targetPos.x - enemy->pos.x );
	Vector2 scale = vec2( enemy->facing, 1.0f );
	spr_UpdateScale( enemy->sprite, &scale );
}

static void createProjectile( Vector2 pos )
{
	Enemy* enemy = createBaseEnemy( ET_PROJECTILE, pos, 1.0f );
	Vector2 dir;
	vec2_Subtract( &player.pos, &pos, &dir );
	vec2_Normalize( &dir );
	vec2_Scale( &dir, rand_GetRangeFloat( NULL, 600.0f, 800.0f ), &enemy->velocity );
	//enemy->isAlive = false;
	playShoot( );
}

static void shooterUpdate( Enemy* enemy )
{
	if( enemy->state == 0 ) {
		// seek target
		float dist = vec2_Dist( &enemy->pos, &enemy->targetPos );
		float speed = lerp( 0.0f, enemy->speed, clamp01( dist / 100.0f ) );
		if( dist < 20.0f ) {
			speed = 0.0f;
			enemy->state = 1;
			enemy->timeAlive = rand_GetRangeFloat( NULL, -5.0f, -3.0f );
			enemy->velocity = VEC2_ZERO;
		} else {
			Vector2 diff;
			vec2_Subtract( &enemy->targetPos, &enemy->pos, &diff );
			vec2_Normalize( &diff );
			vec2_Scale( &diff, speed, &enemy->velocity );
		}
	} else if( enemy->state == 1 ) {
		if( enemy->timeAlive >= 0.0f ) {
			shooterChooseTarget( enemy );
			
			enemy->state = 0;
		}
	}


	if( enemy->otherTimer >= 2.0f ) {
		// shoot a projectile
		createProjectile( enemy->pos );
		enemy->otherTimer = rand_GetRangeFloat( NULL, -1.0f, 0.0f );
	}
}

static void projectileUpdate( Enemy* enemy )
{
	// if we're off the screen then destroy ourselves
	if( enemy->pos.x < -20.0f || enemy->pos.x >= renderWidth + 20.0f ) {
		if( enemy->pos.y < -20.0f || enemy->pos.x >= renderHeight + 20.0f ) {
			enemy->shouldDestroy = true;
		}
	}
}

static EnemyTypeData enemyTypes[] = {
	{ createZombie, "Images/zombie_alive.png", "Images/zombie_dead.png", -1, -1, zombieUpdate, 1 },
	{ createPenanggalan, "Images/penanggalan_alive.png", "Images/penanggalan_dead.png", -1, -1, penanggalanUpdate, 2 },
	{ createJumper, "Images/hopper_alive.png", "Images/hopper_dead.png", -1, -1, jumperUpdate, 4 },
	{ createShooter, "Images/shooter_alive.png", "Images/shooter_dead.png", -1, -1, shooterUpdate, 6 },
	{ NULL, "Images/projectile.png", "Images/projectile.png", -1, -1, projectileUpdate, -1 },
};

static void updateEnemyCollider( Enemy* enemy )
{
	switch( enemyColliderTypes[enemy->type].type ) {
	case CT_AABB:
		vec2_Add( &( enemyColliderTypes[enemy->type].aabb.center ), &( enemy->pos ), &( enemy->collision.aabb.center ) );
		break;
	case CT_CIRCLE:
		vec2_Add( &( enemyColliderTypes[enemy->type].circle.center ), &( enemy->pos ), &( enemy->collision.circle.center ) );
		break;
	}
}

static void updateEnemyCollisionCollection( void )
{
	enemyColliderCollection.count = sb_Count( sbEnemies );
	if( enemyColliderCollection.count == 0 ) {
		enemyColliderCollection.firstCollider = NULL;
	} else {
		enemyColliderCollection.firstCollider = &( sbEnemies[0].collision );
	}
}

static void cleanUpAllEnemies( void )
{
	for( size_t i = 0; i < sb_Count( sbEnemies ); ++i ) {
		spr_DestroySprite( sbEnemies[i].sprite );
	}
	sb_Clear( sbEnemies );
	updateEnemyCollisionCollection( );
}

static void cleanUpEnemy( size_t idx )
{
	spr_DestroySprite( sbEnemies[idx].sprite );
	sb_Remove( sbEnemies, idx );
	updateEnemyCollisionCollection( );
}

static void enemyPhysicsTick( float dt )
{
	enemyDT = dt;

	for( size_t i = 0; i < sb_Count( sbEnemies ); ++i ) {
		vec2_AddScaled( &( sbEnemies[i].pos ), &( sbEnemies[i].velocity ), dt, &( sbEnemies[i].pos ) );
		updateEnemyCollider( &sbEnemies[i] );
		sbEnemies[i].timeAlive += dt;
		sbEnemies[i].otherTimer += dt;

		if( sbEnemies[i].isAlive ) {
			if( enemyTypes[sbEnemies[i].type].update != NULL ) {
				enemyTypes[sbEnemies[i].type].update( &sbEnemies[i] );
			}
		} else {
			deadEnemyUpdate( &sbEnemies[i] );
		}
	}

	for( size_t i = 0; i < sb_Count( sbEnemies ); ++i ) {
		if( sbEnemies[i].shouldDestroy ) {
			cleanUpEnemy( i );
			--i;
		}
	}
}

static void drawEnemies( )
{
	for( size_t i = 0; i < sb_Count( sbEnemies ); ++i ) {
		if( sbEnemies[i].snapNewPos ) {
			spr_SnapPos( sbEnemies[i].sprite, &sbEnemies[i].pos );
			sbEnemies[i].snapNewPos = false;
		} else {
			spr_UpdatePos( sbEnemies[i].sprite, &sbEnemies[i].pos );
		}
	}
}

static bool killEnemy( int idx, Vector2 separation )
{
	if( !sbEnemies[idx].isAlive ) return false;

	sbEnemies[idx].isAlive = false;
	spr_SwitchImage( sbEnemies[idx].sprite, enemyTypes[sbEnemies[idx].type].deadImg );
	sbEnemies[idx].timeAlive = 0.0f;
	sbEnemies[idx].facing = sign( separation.x );
	Vector2 scale = vec2( sbEnemies[idx].facing, 1.0f );
	spr_UpdateScale( sbEnemies[idx].sprite, &scale );
	sbEnemies[idx].velocity = vec2( -sbEnemies[idx].facing * rand_GetRangeFloat( NULL, 200.0f, 400.0f ), rand_GetRangeFloat( NULL, -600.0f, -800.0f ) );
	sbEnemies[idx].collision.type = CT_DEACTIVATED;

	return true;
}

static Enemy* createBaseEnemy( EnemyType type, Vector2 pos, float facing )
{
	Enemy enemy;

	enemy.timeAlive = 0.0f;
	enemy.pos = pos;
	enemy.facing = facing;
	enemy.isAlive = true;
	enemy.sprite = spr_CreateSprite( enemyTypes[type].aliveImg, 1, pos, vec2( facing, 1.0f ), 0.0f, CLR_WHITE, 9 );
	enemy.shouldDestroy = false;
	enemy.snapNewPos = false;

	enemy.type = type;
	enemy.collision = enemyColliderTypes[type];

	sb_Push( sbEnemies, enemy );

	updateEnemyCollisionCollection( );

	return &( sb_Last( sbEnemies ) );
}

static void chooseSide( float* facing, float* xPos )
{
	if( rand_Choice( NULL ) ) {
		// right
		(*xPos) = renderWidth + 32.0f;
		(*facing) = -1.0f;
	} else {
		// left
		(*xPos) = -32.0f;
		(*facing) = 1.0f;
	}
}

static void createZombie( void )
{
	// zombies spawn on one of the edges facing towards the center, and move that direction
	Vector2 spawnPos;
	spawnPos.y = renderHeight - 96.0f;

	float facing;
	chooseSide( &facing, &spawnPos.x );

	Enemy* nmy = createBaseEnemy( ET_ZOMBIE, spawnPos, facing );
	float speed = rand_GetRangeFloat( NULL, 100.0f, 200.0f );
	nmy->velocity = vec2( facing * speed, 0.0f );
}

static void createPenanggalan( void )
{
	Vector2 spawnPos;
	spawnPos.y = rand_GetRangeFloat( NULL, 100.0f, 300.0f );
	float facing;
	chooseSide( &facing, &spawnPos.x );

	Enemy* nmy = createBaseEnemy( ET_PENANGGALAN, spawnPos, facing );
	nmy->velocity = vec2( facing * 300.0f, 0.0f );

	nmy->timeAlive = rand_GetRangeFloat( NULL, 0.0f, 10.0f );
}

static void createJumper( void )
{
	Vector2 spawnPos;
	spawnPos.y = renderHeight - 96.0f;

	float facing;
	chooseSide( &facing, &spawnPos.x );

	Enemy* nmy = createBaseEnemy( ET_JUMPER, spawnPos, facing );
	nmy->speed = rand_GetRangeFloat( NULL, 200.0f, 200.0f );
	nmy->velocity = VEC2_ZERO;
	nmy->state = rand_Choice( NULL ) ? 0 : 1;
}

static void createShooter( void )
{
	Vector2 spawnPos;
	spawnPos.y = -96.0f;
	spawnPos.x = rand_GetRangeFloat( NULL, 0.0f, (float)renderWidth );

	float facing = rand_Choice( NULL ) ? -1.0f : 1.0f;

	Enemy* nmy = createBaseEnemy( ET_SHOOTER, spawnPos, facing );
	nmy->speed = 400.0f;
	nmy->velocity = VEC2_ZERO;
	nmy->state = 0;
	nmy->timeAlive = rand_GetRangeFloat( NULL, -3.5f, -1.5f );
	nmy->otherTimer = rand_GetRangeFloat( NULL, -2.0f, -4.0f );

	shooterChooseTarget( nmy );
}

static void loadEnemies( void )
{
	for( size_t i = 0; i < ARRAY_SIZE( enemyTypes ); ++i ) {
		enemyTypes[i].aliveImg = img_Load( enemyTypes[i].aliveImgPath, ST_DEFAULT );
		enemyTypes[i].deadImg = img_Load( enemyTypes[i].deadImgPath, ST_DEFAULT );
	}

	enemyColliderTypes[ET_ZOMBIE].type = CT_AABB;
	enemyColliderTypes[ET_ZOMBIE].aabb.center = VEC2_ZERO;
	enemyColliderTypes[ET_ZOMBIE].aabb.halfDim = vec2( 10.0f, 32.0f );

	enemyColliderTypes[ET_PENANGGALAN].type = CT_CIRCLE;
	enemyColliderTypes[ET_PENANGGALAN].circle.center = VEC2_ZERO;
	enemyColliderTypes[ET_PENANGGALAN].circle.radius = 14.0f;

	enemyColliderTypes[ET_JUMPER].type = CT_AABB;
	enemyColliderTypes[ET_JUMPER].aabb.center = VEC2_ZERO;
	enemyColliderTypes[ET_JUMPER].aabb.halfDim = vec2( 10.0f, 32.0f );

	enemyColliderTypes[ET_SHOOTER].type = CT_AABB;
	enemyColliderTypes[ET_SHOOTER].aabb.center = VEC2_ZERO;
	enemyColliderTypes[ET_SHOOTER].aabb.halfDim = vec2( 10.0f, 32.0f );

	enemyColliderTypes[ET_PROJECTILE].type = CT_CIRCLE;
	enemyColliderTypes[ET_PROJECTILE].circle.center = VEC2_ZERO;
	enemyColliderTypes[ET_PROJECTILE].circle.radius = 10.0f;
}

typedef struct {
	EntityID sprite;
	Collider collider;
	float rotVel;
	float timeAlive;

	Tween scaleTween;
	Tween alphaTween;
} PlayerExplosion;

static PlayerExplosion* sbExplosions = NULL;
static ColliderCollection explosionColliders = { NULL, sizeof( PlayerExplosion ), 0 };

static void refreshExplosionColliders( void )
{
	explosionColliders.count = sb_Count( sbExplosions );
	if( explosionColliders.count == 0 ) {
		explosionColliders.firstCollider = NULL;
	} else {
		explosionColliders.firstCollider = &sbExplosions[0].collider;
	}
}

static void createExplosionEffect( Vector2 pos, float power )
{
	PlayerExplosion explosion;

	float maxRadius = power;

	Vector2 scale = VEC2_ZERO;
	float rot = rand_GetRangeFloat( NULL, 0.0f, M_TWO_PI_F );

	explosion.sprite = spr_CreateSprite( explosionImg, 1, pos, scale, rot, CLR_RED, -1 );
	explosion.rotVel = rand_GetRangeFloat( NULL, M_PI_F, M_TWO_PI_F ) * ( rand_Choice( NULL ) ? 1.0f : -1.0f );

	const float MAX_TIME = 0.25f;
	setTween( &explosion.scaleTween, 0.0f, maxRadius, MAX_TIME, easeOutBack );
	setTween( &explosion.alphaTween, 1.0f, 0.0f, MAX_TIME, easeInCirc );

	explosion.collider.type = CT_CIRCLE;
	explosion.collider.circle.center = pos;
	explosion.collider.circle.radius = 0.0f;

	sb_Push( sbExplosions, explosion );

	refreshExplosionColliders( );
}

static void createExplosion( Vector2 pos, float power )
{
	createExplosionEffect( pos, power );
	createExplosionEffect( pos, power * ( 2.0f / 3.0f ) );
	createExplosionEffect( pos, power * ( 1.0f / 3.0f ) );
	playExplosion( clamp01( power / 4.0f ) );
}

static void updateExplosions( float dt )
{
	for( size_t i = 0; i < sb_Count( sbExplosions ); ++i ) {
		bool isDone = processTween( &( sbExplosions[i].alphaTween ), dt );
		isDone = processTween( &( sbExplosions[i].scaleTween ), dt ) && isDone;

		if( isDone ) {
			spr_DestroySprite( sbExplosions[i].sprite );
			sb_Remove( sbExplosions, i );
			--i;
			refreshExplosionColliders( );
			continue;
		}

		spr_UpdateRot_Delta( sbExplosions[i].sprite, sbExplosions[i].rotVel * dt );
		Color clr = CLR_RED;
		clr.a = sbExplosions[i].alphaTween.current;
		spr_UpdateColor( sbExplosions[i].sprite, &clr );

		Vector2 scale = vec2( sbExplosions[i].scaleTween.current, sbExplosions[i].scaleTween.current );
		spr_UpdateScale( sbExplosions[i].sprite, &scale );

		sbExplosions[i].collider.circle.radius = 64.0f * sbExplosions[i].scaleTween.current;
	}
}

static void drawExplosionColliders( void )
{
	for( size_t i = 0; i < sb_Count( sbExplosions ); ++i ) {
		collision_CollectionDebugDrawing( explosionColliders, 1, CLR_RED );
	}
}

typedef struct {
	const char* spriteFileName;
	Vector2 spriteOffset;
	void ( *handleLeftInput )( void );
	void ( *handleRightInput )( void );
	void ( *handleUpInput )( void );
	void ( *handleDownInput )( void );
	void ( *stateEnter )( void );
	void ( *stateUpdate )( void );
	void ( *stateHitWithAttack )( void );
	void ( *stateHitGround )( void );
} CharacterStateTemplate;

typedef struct {
	int spriteImg;
	Collider bodyCollider;
	Collider weaponCollider;
	Collider damageCollider;
	bool weaponActive;
} CharacterStateInfo;

static CharacterStateInfo playerInfos[NUM_CHARACTER_STATES];

static void setPlayerState( CharacterState newState );


static void playerSlash_Enter( void )
{
	playAttack( );
	player.velocity = vec2( player.facing * 900.0f, 0.0f );
}

static void playerSlash_Update( void )
{
	if( player.timeInState >= 0.2f ) {
		setPlayerState( CS_IDLE );
	}
}

static void playerAirSlash_Update( void )
{
	if( player.timeInState >= 0.2f ) {
		setPlayerState( CS_AIR_IDLE );
	}
}

static void playerJumpUp_Enter( void )
{
	playAttack( );
	player.velocity = vec2( 0.0f, -900.0f );
	player.canAirJump = false;
}

static void playerJumpUp_Update( void )
{
	if( player.timeInState >= 0.25f ) {
		setPlayerState( CS_AIR_IDLE );
	}
}

static void setPlayerFacing( float facing )
{
	player.facing = facing;
	Vector2 facingScale = vec2( facing, 1.0f );
	spr_UpdateScale( player.sprite, &facingScale );
}

static void playerIdle_Enter( void )
{
	player.velocity = VEC2_ZERO;
}

static void playerIdle_PressLeft( void )
{
	if( player.timeInState <= 0.05f ) return;
	setPlayerFacing( -1.0f );
	setPlayerState( player.slashOne ? CS_SLASH_1 : CS_SLASH_2 );
	player.slashOne = !player.slashOne;
}

static void playerIdle_PressRight( void )
{
	if( player.timeInState <= 0.05f ) return;
	setPlayerFacing( 1.0f );
	setPlayerState( player.slashOne ? CS_SLASH_1 : CS_SLASH_2 );
	player.slashOne = !player.slashOne;
}

static void playerIdle_PressUp( void )
{
	if( player.timeInState <= 0.05f ) return;
	setPlayerState( CS_JUMP_UP );
}

static void playerIdle_PressDown( void )
{

}

static void playerAirIdle_Enter( void )
{
	player.velocity = VEC2_ZERO;
}

static void playerAirIdle_Update( void )
{
	if( player.timeInState >= 0.2f ) {
		setPlayerState( CS_FALL );
	}
}

static void playerAirIdle_PressLeft( void )
{
	setPlayerFacing( -1.0f );
	setPlayerState( player.slashOne ? CS_AIR_SLASH_1 : CS_AIR_SLASH_2 );
	player.slashOne = !player.slashOne;
}

static void playerAirIdle_PressRight( void )
{
	setPlayerFacing( 1.0f );
	setPlayerState( player.slashOne ? CS_AIR_SLASH_1 : CS_AIR_SLASH_2 );
	player.slashOne = !player.slashOne;
}

static void playerAirIdle_PressUp( void )
{
	if( player.canAirJump ) {
		setPlayerState( CS_JUMP_UP );
	}
}

static void playerAirIdle_PressDown( void )
{
	setPlayerState( CS_FALL_ATTACK );
}

static void playerFall_Enter( void )
{

}

static void playerFall_Update( void )
{
	player.velocity = vec2( 0.0f, player.timeInState * 900.0f );
}

static void playerFall_HitGround( void )
{
	setPlayerState( CS_LAND );
}

static void playerLand_Enter( void )
{
	player.velocity = VEC2_ZERO;
}

static void playerLand_Update( void )
{
	if( player.timeInState >= 0.2f ) {
		setPlayerState( CS_IDLE );
	}
}

static void playerFallAttack_Enter( void )
{
	playAttack( );
	player.heightAtAttackStart = player.pos.y;
	player.velocity = vec2( 0.0f, 900.0f );
}

static void playerFallAttack_HitGround( void )
{
	setPlayerState( CS_LAND );

	// spawn explosion based on height from attack start
	float diff = player.pos.y - player.heightAtAttackStart;
	float power = floorf( diff / 100.0f );

	createExplosion( player.pos, power );
}

static CharacterStateTemplate playerInfoTemplates[] = {
	{ "Images/player_idle.png", { 48.0f, 16.0f }, playerIdle_PressLeft, playerIdle_PressRight, playerIdle_PressUp, playerIdle_PressDown, playerIdle_Enter, NULL, NULL, NULL },
	{ "Images/player_slash1.png", { 48.0f, 16.0f }, NULL, NULL, NULL, NULL, playerSlash_Enter, playerSlash_Update, NULL, NULL },
	{ "Images/player_slash2.png", { 48.0f, 16.0f }, NULL, NULL, NULL, NULL, playerSlash_Enter, playerSlash_Update, NULL, NULL },
	{ "Images/player_jumpUp.png", { 48.0f, 16.0f }, NULL, NULL, NULL, NULL, playerJumpUp_Enter, playerJumpUp_Update, NULL, NULL },
	{ "Images/player_airIdle.png", { 48.0f, 16.0f }, playerAirIdle_PressLeft, playerAirIdle_PressRight, playerAirIdle_PressUp, playerAirIdle_PressDown, playerAirIdle_Enter, playerAirIdle_Update, NULL, NULL },
	{ "Images/player_fall.png", { 48.0f, 16.0f }, playerAirIdle_PressLeft, playerAirIdle_PressRight, playerAirIdle_PressUp, playerAirIdle_PressDown, playerFall_Enter, playerFall_Update, NULL, playerFall_HitGround },
	{ "Images/player_land.png", { 48.0f, 16.0f }, NULL, NULL, NULL, NULL, playerLand_Enter, playerLand_Update, NULL, NULL },
	{ "Images/player_airSlash1.png", { 48.0f, 16.0f }, NULL, NULL, NULL, NULL, playerSlash_Enter, playerAirSlash_Update, NULL, NULL },
	{ "Images/player_airSlash2.png", { 48.0f, 16.0f }, NULL, NULL, NULL, NULL, playerSlash_Enter, playerAirSlash_Update, NULL, NULL },
	{ "Images/player_fallAttack.png", { 48.0f, 16.0f }, NULL, NULL, NULL, NULL, playerFallAttack_Enter, NULL, NULL, playerFallAttack_HitGround },
	{ "Images/player_death.png", { 48.0f, 16.0f }, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
};

static void setPlayerState( CharacterState newState )
{
	spr_SwitchImage( player.sprite, playerInfos[newState].spriteImg );
	player.state = newState;
	player.timeInState = 0.0f;
	if( playerInfoTemplates[newState].stateEnter != NULL ) {
		playerInfoTemplates[newState].stateEnter( );
	}
}

static void pressUp( void )
{
	if( playerInfoTemplates[player.state].handleUpInput != NULL ) {
		playerInfoTemplates[player.state].handleUpInput( );
	}
}

static void releaseUp( void )
{

}

static void pressDown( void )
{
	if( playerInfoTemplates[player.state].handleDownInput != NULL ) {
		playerInfoTemplates[player.state].handleDownInput( );
	}
}

static void releaseDown( void )
{

}

static void pressLeft( void )
{
	if( playerInfoTemplates[player.state].handleLeftInput != NULL ) {
		playerInfoTemplates[player.state].handleLeftInput( );
	}
}

static void releaseLeft( void )
{

}

static void pressRight( void )
{
	if( playerInfoTemplates[player.state].handleRightInput != NULL ) {
		playerInfoTemplates[player.state].handleRightInput( );
	}
}

static void releaseRight( void )
{

}

static void bindPlayerKeys( void )
{
	input_BindOnKey( SDLK_UP, pressUp, releaseUp );
	input_BindOnKey( SDLK_w, pressUp, releaseUp );

	input_BindOnKey( SDLK_DOWN, pressDown, releaseDown );
	input_BindOnKey( SDLK_s, pressDown, releaseDown );

	input_BindOnKey( SDLK_LEFT, pressLeft, releaseLeft );
	input_BindOnKey( SDLK_a, pressLeft, releaseLeft );

	input_BindOnKey( SDLK_RIGHT, pressRight, releaseRight );
	input_BindOnKey( SDLK_d, pressRight, releaseRight );
}

static void startTimer( void )
{
	secondTimer = 1.0f;
	secondCount = 0;
}

static void loadCharacterInfos( CharacterStateInfo* infos, CharacterStateTemplate* templates, size_t count )
{
	for( size_t i = 0; i < count; ++i ) {
		infos[i].spriteImg = img_Load( templates[i].spriteFileName, ST_DEFAULT );
		img_SetOffset( infos[i].spriteImg, templates[i].spriteOffset );
	}
}

static void loadPlayer( void )
{
	size_t count = MIN( ARRAY_SIZE( playerInfos ), ARRAY_SIZE( playerInfoTemplates ) );
	loadCharacterInfos( playerInfos, playerInfoTemplates, count );

	playerInfos[CS_IDLE].bodyCollider.type = CT_AABB;
	playerInfos[CS_IDLE].bodyCollider.aabb.center = vec2( 0.0f, 0.0f );
	playerInfos[CS_IDLE].bodyCollider.aabb.halfDim = vec2( 16.0f, 32.0f );
	playerInfos[CS_IDLE].weaponActive = false;
	playerInfos[CS_IDLE].damageCollider.type = CT_AABB;
	playerInfos[CS_IDLE].damageCollider.aabb.center = vec2( 0.0f, 0.0f );
	playerInfos[CS_IDLE].damageCollider.aabb.halfDim = vec2( 8.0f, 16.0f );

	playerInfos[CS_SLASH_1].bodyCollider.type = CT_AABB;
	playerInfos[CS_SLASH_1].bodyCollider.aabb.center = vec2( -16.0f, 0.0f );
	playerInfos[CS_SLASH_1].bodyCollider.aabb.halfDim = vec2( 16.0f, 32.0f );
	playerInfos[CS_SLASH_1].weaponActive = true;
	playerInfos[CS_SLASH_1].weaponCollider.type = CT_AABB;
	playerInfos[CS_SLASH_1].weaponCollider.aabb.center = vec2( 32.0f, -3.0f );
	playerInfos[CS_SLASH_1].weaponCollider.aabb.halfDim = vec2( 34.0f, 25.0f );
	playerInfos[CS_SLASH_1].damageCollider.type = CT_AABB;
	playerInfos[CS_SLASH_1].damageCollider.aabb.center = vec2( -16.0f, 0.0f );
	playerInfos[CS_SLASH_1].damageCollider.aabb.halfDim = vec2( 8.0f, 16.0f );

	playerInfos[CS_SLASH_2].bodyCollider.type = CT_AABB;
	playerInfos[CS_SLASH_2].bodyCollider.aabb.center = vec2( -16.0f, 0.0f );
	playerInfos[CS_SLASH_2].bodyCollider.aabb.halfDim = vec2( 16.0f, 32.0f );
	playerInfos[CS_SLASH_2].weaponActive = true;
	playerInfos[CS_SLASH_2].weaponCollider.type = CT_AABB;
	playerInfos[CS_SLASH_2].weaponCollider.aabb.center = vec2( 32.0f, -3.0f );
	playerInfos[CS_SLASH_2].weaponCollider.aabb.halfDim = vec2( 34.0f, 25.0f );
	playerInfos[CS_SLASH_2].damageCollider.type = CT_AABB;
	playerInfos[CS_SLASH_2].damageCollider.aabb.center = vec2( -16.0f, 0.0f );
	playerInfos[CS_SLASH_2].damageCollider.aabb.halfDim = vec2( 8.0f, 16.0f );

	playerInfos[CS_JUMP_UP].bodyCollider.type = CT_AABB;
	playerInfos[CS_JUMP_UP].bodyCollider.aabb.center = vec2( 0.0f, 16.0f );
	playerInfos[CS_JUMP_UP].bodyCollider.aabb.halfDim = vec2( 16.0f, 32.0f );
	playerInfos[CS_JUMP_UP].weaponActive = true;
	playerInfos[CS_JUMP_UP].weaponCollider.type = CT_AABB;
	playerInfos[CS_JUMP_UP].weaponCollider.aabb.center = vec2( 26.0f, 16.0f );
	playerInfos[CS_JUMP_UP].weaponCollider.aabb.halfDim = vec2( 20.0f, 32.0f );
	playerInfos[CS_JUMP_UP].damageCollider.type = CT_AABB;
	playerInfos[CS_JUMP_UP].damageCollider.aabb.center = vec2( 0.0f, 16.0f );
	playerInfos[CS_JUMP_UP].damageCollider.aabb.halfDim = vec2( 8.0f, 16.0f );

	playerInfos[CS_AIR_IDLE].bodyCollider.type = CT_AABB;
	playerInfos[CS_AIR_IDLE].bodyCollider.aabb.center = vec2( 0.0f, 0.0f );
	playerInfos[CS_AIR_IDLE].bodyCollider.aabb.halfDim = vec2( 16.0f, 32.0f );
	playerInfos[CS_AIR_IDLE].weaponActive = false;
	playerInfos[CS_AIR_IDLE].damageCollider.type = CT_AABB;
	playerInfos[CS_AIR_IDLE].damageCollider.aabb.center = vec2( 0.0f, 0.0f );
	playerInfos[CS_AIR_IDLE].damageCollider.aabb.halfDim = vec2( 8.0f, 16.0f );

	playerInfos[CS_FALL].bodyCollider.type = CT_AABB;
	playerInfos[CS_FALL].bodyCollider.aabb.center = vec2( 0.0f, 0.0f );
	playerInfos[CS_FALL].bodyCollider.aabb.halfDim = vec2( 16.0f, 32.0f );
	playerInfos[CS_FALL].weaponActive = false;
	playerInfos[CS_FALL].damageCollider.type = CT_AABB;
	playerInfos[CS_FALL].damageCollider.aabb.center = vec2( 0.0f, 0.0f );
	playerInfos[CS_FALL].damageCollider.aabb.halfDim = vec2( 8.0f, 16.0f );

	playerInfos[CS_LAND].bodyCollider.type = CT_AABB;
	playerInfos[CS_LAND].bodyCollider.aabb.center = vec2( 16.0f, 16.0f );
	playerInfos[CS_LAND].bodyCollider.aabb.halfDim = vec2( 16.0f, 16.0f );
	playerInfos[CS_LAND].weaponActive = false;
	playerInfos[CS_LAND].damageCollider.type = CT_AABB;
	playerInfos[CS_LAND].damageCollider.aabb.center = vec2( 16.0f, 16.0f );
	playerInfos[CS_LAND].damageCollider.aabb.halfDim = vec2( 8.0f, 8.0f );

	playerInfos[CS_AIR_SLASH_1].bodyCollider.type = CT_AABB;
	playerInfos[CS_AIR_SLASH_1].bodyCollider.aabb.center = vec2( -16.0f, 0.0f );
	playerInfos[CS_AIR_SLASH_1].bodyCollider.aabb.halfDim = vec2( 16.0f, 32.0f );
	playerInfos[CS_AIR_SLASH_1].weaponActive = true;
	playerInfos[CS_AIR_SLASH_1].weaponCollider.type = CT_AABB;
	playerInfos[CS_AIR_SLASH_1].weaponCollider.aabb.center = vec2( 32.0f, -3.0f );
	playerInfos[CS_AIR_SLASH_1].weaponCollider.aabb.halfDim = vec2( 34.0f, 25.0f );
	playerInfos[CS_AIR_SLASH_1].damageCollider.type = CT_AABB;
	playerInfos[CS_AIR_SLASH_1].damageCollider.aabb.center = vec2( -16.0f, 0.0f );
	playerInfos[CS_AIR_SLASH_1].damageCollider.aabb.halfDim = vec2( 8.0f, 16.0f );

	playerInfos[CS_AIR_SLASH_2].bodyCollider.type = CT_AABB;
	playerInfos[CS_AIR_SLASH_2].bodyCollider.aabb.center = vec2( -16.0f, 0.0f );
	playerInfos[CS_AIR_SLASH_2].bodyCollider.aabb.halfDim = vec2( 16.0f, 32.0f );
	playerInfos[CS_AIR_SLASH_2].weaponActive = true;
	playerInfos[CS_AIR_SLASH_2].weaponCollider.type = CT_AABB;
	playerInfos[CS_AIR_SLASH_2].weaponCollider.aabb.center = vec2( 32.0f, -3.0f );
	playerInfos[CS_AIR_SLASH_2].weaponCollider.aabb.halfDim = vec2( 34.0f, 25.0f );
	playerInfos[CS_AIR_SLASH_2].damageCollider.type = CT_AABB;
	playerInfos[CS_AIR_SLASH_2].damageCollider.aabb.center = vec2( -16.0f, 0.0f );
	playerInfos[CS_AIR_SLASH_2].damageCollider.aabb.halfDim = vec2( 8.0f, 16.0f );

	playerInfos[CS_FALL_ATTACK].bodyCollider.type = CT_AABB;
	playerInfos[CS_FALL_ATTACK].bodyCollider.aabb.center = vec2( 0.0f, 0.0f );
	playerInfos[CS_FALL_ATTACK].bodyCollider.aabb.halfDim = vec2( 16.0f, 32.0f );
	playerInfos[CS_FALL_ATTACK].weaponActive = true;
	playerInfos[CS_FALL_ATTACK].weaponCollider.type = CT_AABB;
	playerInfos[CS_FALL_ATTACK].weaponCollider.aabb.center = vec2( 0.0f, 32.0f );
	playerInfos[CS_FALL_ATTACK].weaponCollider.aabb.halfDim = vec2( 24.0f, 40.0f );
	playerInfos[CS_FALL_ATTACK].damageCollider.type = CT_AABB;
	playerInfos[CS_FALL_ATTACK].damageCollider.aabb.center = vec2( 0.0f, 0.0f );
	playerInfos[CS_FALL_ATTACK].damageCollider.aabb.halfDim = vec2( 8.0f, 16.0f );
}

static void createPlayer( void )
{
	player.pos = vec2( renderWidth / 2.0f, renderHeight - 96.0f );
	player.state = CS_IDLE;
	player.facing = 1.0f;
	player.velocity = VEC2_ZERO;

	player.sprite = spr_CreateSprite( playerInfos[player.state].spriteImg, 1, player.pos, VEC2_ONE, 0.0f, CLR_WHITE, 10 );

	player.slashOne = true;
}

static void destroyPlayer( void )
{
	spr_DestroySprite( player.sprite );
}

static void updatePlayer( void )
{
	player.bodyCollider = playerInfos[player.state].bodyCollider;
	player.weaponCollider = playerInfos[player.state].weaponCollider;
	player.damageCollider = playerInfos[player.state].damageCollider;

	vec2_AddScaled( &( player.pos ), &( player.bodyCollider.aabb.center ), player.facing, &( player.bodyCollider.aabb.center ) );
	vec2_AddScaled( &( player.pos ), &( player.weaponCollider.aabb.center ), player.facing, &( player.weaponCollider.aabb.center ) );
	vec2_AddScaled( &( player.pos ), &( player.damageCollider.aabb.center ), player.facing, &( player.damageCollider.aabb.center ) );

	if( playerInfoTemplates[player.state].stateUpdate != NULL ) {
		playerInfoTemplates[player.state].stateUpdate( );
	}
}

static void drawPlayer( void )
{
	spr_UpdatePos( player.sprite, &player.pos );
}

static void createArenaColliders( void )
{
	Vector2 groundTop = vec2( renderWidth / 2.0f, renderHeight - 64.0f );
	Vector2 groundNormal = VEC2_UP;
	collision_CalculateHalfSpace( &groundTop, &groundNormal, &bottomArenaCollider );

	Vector2 left = vec2( 10.0f, 0.0f );
	Vector2 leftNormal = VEC2_RIGHT;
	collision_CalculateHalfSpace( &left, &leftNormal, &leftArenaCollider );

	Vector2 right = vec2( (float)renderWidth - 10.0f, 0.0f );
	Vector2 rightNormal = VEC2_LEFT;
	collision_CalculateHalfSpace( &right, &rightNormal, &rightArenaCollider );

	Vector2 top = vec2( 0.0f, 10.0f );
	Vector2 topNormal = VEC2_DOWN;
	collision_CalculateHalfSpace( &top, &topNormal, &topArenaCollider );
}

#define MAX_ENEMY_TYPES 10


typedef struct {
	EnemyType type;
	float timeLeft;
} SpawnInstruction;

static SpawnInstruction* sbSpawns = NULL;

static EnemyType chooseEnemyType( int maxScore )
{
	EnemyType type = NUM_ENEMY_TYPES;

	int numChecked = 0;
	for( size_t i = 0; i < ARRAY_SIZE( enemyTypes ); ++i ) {
		if( enemyTypes[i].typeScore <= 0 ) continue;
		if( enemyTypes[i].typeScore > maxScore ) continue;

		if( rand_GetRangeS32( NULL, 0, numChecked ) == 0 ) {
			type = i;
		}
		++numChecked;
	}

	return type;
}

static int spawnIncrease = 3;
static int spawnThresholdMult = 4;
static int spawnTotalBase = 2;
static int spawnTotalMult = 2;

static void initSpawnValues( void )
{
	maxSpawnScore = 1;
	totalSpawnScore = 4;
}

static void createSpawnGroup( void )
{
	// have two values, total score and highest score which vary over time
	//  highest score is the highest enemy type score that can be spawned
	//  total score is the sum of the enemy type scores that will be spawned
	int scoreLeft = totalSpawnScore;
	int i = 0;
	while( scoreLeft > 0 ) {
		SpawnInstruction inst;
		inst.type = chooseEnemyType( MIN( scoreLeft, maxSpawnScore ) );
		inst.timeLeft = i * 1.0f;
		sb_Push( sbSpawns, inst );
		++i;
		scoreLeft -= enemyTypes[inst.type].typeScore;
	}

	if( maxSpawnScore >= 6 ) {
		// throw a few zombies in when the max score gets high
		int extras = rand_GetRangeS32( NULL, 2, 6 );
		SpawnInstruction inst;
		inst.type = ET_ZOMBIE;
		inst.timeLeft = i * 1.0f;
		sb_Push( sbSpawns, inst );
		++i;
	}

	// increase spawn variables
	totalSpawnScore += spawnIncrease;
	if( totalSpawnScore > ( maxSpawnScore * spawnThresholdMult ) ) {
		maxSpawnScore += 1;
		totalSpawnScore = spawnTotalBase + ( maxSpawnScore * spawnTotalMult );
	}
}

static void clearSpawnInstructions( void )
{
	sb_Clear( sbSpawns );
}

static void processSpawnInstructions( float dt )
{
	for( size_t i = 0; i < sb_Count( sbSpawns ); ++i ) {
		sbSpawns[i].timeLeft -= dt;
		if( sbSpawns[i].timeLeft <= 0.0f ) {
			enemyTypes[sbSpawns[i].type].create( );
			sb_Remove( sbSpawns, i );
			--i;
		}
	}
}

static void processSecondCount( void )
{

}

static void processTenSecondCount( void )
{
	createSpawnGroup( );
}

//****************************************************************************************************

static int titleState_Enter( void );
static int titleState_Exit( void );
static void titleState_ProcessEvents( SDL_Event* e );
static void titleState_Process( void );
static void titleState_Draw( void );
static void titleState_PhysicsTick( float dt );
GameState titleState = { titleState_Enter, titleState_Exit, titleState_ProcessEvents,
	titleState_Process, titleState_Draw, titleState_PhysicsTick };

static int playState_Enter( void );
static int playState_Exit( void );
static void playState_ProcessEvents( SDL_Event* e );
static void playState_Process( void );
static void playState_Draw( void );
static void playState_PhysicsTick( float dt );
GameState playState = { playState_Enter, playState_Exit, playState_ProcessEvents,
	playState_Process, playState_Draw, playState_PhysicsTick };

static int gameOverState_Enter( void );
static int gameOverState_Exit( void );
static void gameOverState_ProcessEvents( SDL_Event* e );
static void gameOverState_Process( void );
static void gameOverState_Draw( void );
static void gameOverState_PhysicsTick( float dt );
GameState gameOverState = { gameOverState_Enter, gameOverState_Exit, gameOverState_ProcessEvents,
	gameOverState_Process, gameOverState_Draw, gameOverState_PhysicsTick };

//****************************************************************************************************

GameStateMachine mainStateMachine;

static int titleState_Enter( void )
{
	timeScale = 1.0f;
	createPlayer( );
	return 1;
}

static int titleState_Exit( void )
{
	destroyPlayer( );
	return 1;
}

static void titleState_ProcessEvents( SDL_Event* e )
{
	if( e->type == SDL_KEYDOWN ) {
		if( e->key.repeat == 0 ) {
#ifdef _DEBUG
			switch( e->key.keysym.sym ) {
			case SDLK_1:
				playShoot( );
				break;
			case SDLK_2:
				playAttack( );
				break;
			case SDLK_3:
				playHitEnemy( );
				break;
			case SDLK_4:
				playHitByEnemy( );
				break;
			default:
				gsm_EnterState( &mainStateMachine, &playState );
				break;
			}
#else
			gsm_EnterState( &mainStateMachine, &playState );
#endif
		}
	}
}

static void titleState_Process( void )
{}

static void titleState_Draw( void )
{
	Vector2 titlePos = vec2( renderWidth / 2.0f, 100.0f );
	txt_DisplayString( "OTON", titlePos, CLR_WHITE, HORIZ_ALIGN_CENTER, VERT_ALIGN_CENTER, displayFnt, 1, 100, 200.0f );

	char highScoreText[64];
	SDL_snprintf( highScoreText, 64, "High Score %i", highScore );
	Vector2 scorePos = vec2( renderWidth / 2.0f, ( renderHeight / 2.0f ) - 40.0f );
	txt_DisplayString( highScoreText, scorePos, CLR_ORANGE, HORIZ_ALIGN_CENTER, VERT_ALIGN_CENTER, displayFnt, 1, 100, 100.0f );

	Vector2 startPos = vec2( renderWidth / 2.0f, renderHeight - 40.0f );
	txt_DisplayString( "Press any key to start", startPos, CLR_WHITE, HORIZ_ALIGN_CENTER, VERT_ALIGN_CENTER, displayFnt, 1, 100, 76.0f );

	const char* instructions = "Instructions\n" \
		"- Every 10 seconds a group of enemies will spawn.\n" \
		"- Use the WASD or arrow keys to move/attack.\n" \
		"- You can jump again in the air if you have hit an enemy since your last jump.\n" \
		"- Try to get as many points as possible.";

	txt_DisplayTextArea( (const uint8_t*)instructions, vec2( 20.0f, renderHeight / 2.0f ), vec2( 300.0f, 200.0f ), CLR_WHITE, HORIZ_ALIGN_LEFT, VERT_ALIGN_CENTER, infoFnt, 0, NULL, 1, 100, 18.0f );
}

static void titleState_PhysicsTick( float dt )
{}

//****************************************************************************************************

static int playState_Enter( void )
{
	forceTimeScale = false;
	startTimer( );
	bindPlayerKeys( );
	timeScale = 1.0f;

	createPlayer( );

	cleanUpAllEnemies( );
	initSpawnValues( );

	createSpawnGroup( );

	score = 0;

	return 1;
}

static int playState_Exit( void )
{
	input_ClearAllKeyBinds( );
	sb_Clear( sbSpawns );

	return 1;
}

static void playState_ProcessEvents( SDL_Event* e )
{}

static void playState_Process( void )
{}

static void playState_Draw( void )
{
	// draw play UI

	char scoreText[32];
	SDL_snprintf( scoreText, 32, "%i", score );
	txt_DisplayString( scoreText, vec2( 10.0f, 10.0f ), CLR_WHITE, HORIZ_ALIGN_LEFT, VERT_ALIGN_TOP, displayFnt, 1, 100, 50.0f );
}

static void testPlayerArenaCollisions( void )
{
	Vector2 separation;
	if( collision_GetSeparation( &( player.bodyCollider ), &bottomArenaCollider, &separation ) ) {
		vec2_Add( &( player.pos ), &separation, &( player.pos ) );
		if( playerInfoTemplates[player.state].stateHitGround != NULL ) {
			playerInfoTemplates[player.state].stateHitGround( );
		}
	}
	if( collision_GetSeparation( &( player.bodyCollider ), &leftArenaCollider, &separation ) ) {
		vec2_Add( &( player.pos ), &separation, &( player.pos ) );
	}
	if( collision_GetSeparation( &( player.bodyCollider ), &rightArenaCollider, &separation ) ) {
		vec2_Add( &( player.pos ), &separation, &( player.pos ) );
	}
	if( collision_GetSeparation( &( player.bodyCollider ), &topArenaCollider, &separation ) ) {
		vec2_Add( &( player.pos ), &separation, &( player.pos ) );
	}
}

static void hitEnemy( int firstColliderIdx, int enemyIdx, Vector2 separation )
{
	if( killEnemy( enemyIdx, separation ) ) {
		playHitEnemy( );
		pauseTime( 0.1f );
		player.canAirJump = true;
		player.velocity.x = 0.0f;

		if( sbEnemies[enemyIdx].type != ET_PROJECTILE ) {
			++score;
		}
	}
}

static void hitByEnemy( int first, int enemyIdx, Vector2 separation )
{
#if 1
	if( sbEnemies[enemyIdx].shouldDestroy ) return;
	if( player.state == CS_DEATH ) return;

	// issues with the collision while doing the fall attack, am dying when you shouldn't
	if( ( player.state == CS_FALL_ATTACK ) && ( separation.y < 0.0f ) ) return;

	playHitByEnemy( );
	setPlayerFacing( -sign( separation.x ) );
	player.velocity = vec2( player.facing * -100.0f, -100.0f );
	setPlayerState( CS_DEATH );
	spr_UpdateColor( player.sprite, &CLR_RED );
	gsm_EnterState( &mainStateMachine, &gameOverState );
#endif
}

static void playState_PhysicsTick( float dt )
{
	if( !forceTimeScale ) {
		if( pauseTimeLeft > 0.0f ) {
			pauseTimeLeft -= dt;
			if( pauseTimeLeft <= 0.0f ) {
				timeScale = 1.0f;
			}
		}
	}

	float scaledDT = dt * timeScale;
	player.timeInState += scaledDT;
	secondTimer -= scaledDT;
	if( secondTimer <= 0.0f ) {
		processSecondCount( );

		++secondCount;
		if( secondCount >= 10 ) {
			processTenSecondCount( );
			secondCount = 0;
		}

		while( secondTimer <= 0.0f ) {
			secondTimer += 1.0f;
		}
	}

	vec2_AddScaled( &( player.pos ), &( player.velocity ), scaledDT, &( player.pos ) );
	updatePlayer( );
	enemyPhysicsTick( scaledDT );

	testPlayerArenaCollisions( );

	updateExplosions( dt );

	// test enemies against attacks
	if( playerInfos[player.state].weaponActive ) {
		collision_Detect( &player.weaponCollider, enemyColliderCollection, hitEnemy, -1 );
	}
	collision_DetectAll( explosionColliders, enemyColliderCollection, hitEnemy );

	// test player against enemies
	collision_Detect( &player.damageCollider, enemyColliderCollection, hitByEnemy, -1 );

	processSpawnInstructions( dt );
}

//****************************************************************************************************

static float gameOverInputDelay;
static float gameOverSlowDown;
static bool newHighScore;
#define SLOW_DOWN_TIME 0.5f
static int gameOverState_Enter( void )
{
	newHighScore = false;
	if( score > highScore ) {
		highScore = score;
		newHighScore = true;
#ifndef __EMSCRIPTEN__ 
		void* infoFile = cfg_OpenFile( "info.dat" );
		cfg_SetInt( infoFile, "HighScore", highScore );
		cfg_SaveFile( infoFile );
		cfg_CloseFile( infoFile );
#endif
	}

	forceTimeScale = true;
	gameOverInputDelay = 0.2f;

	gameOverSlowDown = SLOW_DOWN_TIME;

	return 1;
}

static int gameOverState_Exit( void )
{
	cleanUpAllEnemies( );
	destroyPlayer( );

	return 1;
}

static void gameOverState_ProcessEvents( SDL_Event* e )
{
	if( gameOverInputDelay <= 0.0f ) {
		if( e->type == SDL_KEYDOWN ) {
			if( e->key.repeat == 0 ) {
				gsm_EnterState( &mainStateMachine, &playState );
			}
		}
	}
}

static void gameOverState_Process( void )
{}

static void gameOverState_Draw( void )
{
	drawPlayer( );
	drawEnemies( );

	Vector2 titlePos = vec2( renderWidth / 2.0f, 100.0f );
	txt_DisplayString( "GAME OVER", titlePos, CLR_WHITE, HORIZ_ALIGN_CENTER, VERT_ALIGN_CENTER, displayFnt, 1, 100, 200.0f );

	if( gameOverSlowDown <= 0.0f ) {
		char scoreText[128];
		if( newHighScore ) {
			SDL_snprintf( scoreText, 128, "New High Score %i", score );
		} else {
			SDL_snprintf( scoreText, 128, "Score %i\nHigh Score %i", score, highScore );
		}

		txt_DisplayString( scoreText, vec2( renderWidth / 2.0f, ( renderHeight / 2.0f ) + 40.0f ), CLR_ORANGE, HORIZ_ALIGN_CENTER, VERT_ALIGN_CENTER, displayFnt, 1, 100, 100.0f );
	}

	if( gameOverInputDelay <= 0.0f ) {
		Vector2 startPos = vec2( renderWidth / 2.0f, renderHeight - 40.0f );
		txt_DisplayString( "Press any key to play again", startPos, CLR_WHITE, HORIZ_ALIGN_CENTER, VERT_ALIGN_CENTER, displayFnt, 1, 100, 76.0f );
	}
}

static void gameOverState_PhysicsTick( float dt )
{
	if( gameOverSlowDown > 0.0f ) {
		gameOverSlowDown -= dt;
		timeScale = gameOverSlowDown / SLOW_DOWN_TIME;
		//llog( LOG_DEBUG, "scale: %f", timeScale );
		playState_PhysicsTick( dt );
	} else {
		if( gameOverInputDelay > 0.0f ) {
			gameOverInputDelay -= dt;
		}
	}
}

//****************************************************************************************************

static int otonMainScreen_Enter( void )
{
#ifndef __EMSCRIPTEN__ 
	void* infoFile = cfg_OpenFile( "info.dat" );
	highScore = cfg_GetInt( infoFile, "Highscore", 0 );
	cfg_CloseFile( infoFile );
#endif

	gfx_GetRenderSize( &renderWidth, &renderHeight );

	cam_TurnOnFlags( 0, 1 );

	gfx_SetClearColor( clr( 0.1f, 0.1f, 0.1f, 1.0f ) );

	displayFnt = txt_CreateSDFFont( "Fonts/KOMIKAX_.ttf" );
	infoFnt = txt_CreateSDFFont( "Fonts/Roboto-Regular.ttf" );

	spr_Init( );

	whiteImg = img_Load( "Images/white.png", ST_DEFAULT );
	explosionImg = img_Load( "Images/explosion_fx.png", ST_DEFAULT );

	createArenaColliders( );
	loadPlayer( );
	loadEnemies( );
	loadSounds( );
	loadBG( );

	gsm_EnterState( &mainStateMachine, &titleState );

	return 1;
}

static int otonMainScreen_Exit( void )
{
	spr_CleanUp( );

	return 1;
}

static void otonMainScreen_ProcessEvents( SDL_Event* e )
{
	gsm_ProcessEvents( &mainStateMachine, e );
}

static void otonMainScreen_Process( void )
{
	gsm_Process( &mainStateMachine );
}

static void otonMainScreen_Draw( void )
{
	// draw background

	// draw floor
	Vector2 floorPos = vec2( renderWidth / 2.0f, renderHeight - 34.0f );
	Vector2 floorSize = vec2( (float)renderWidth, 68.0f );
	Color floorClr = clr( 0.0f, 0.3f, 0.0f, 1.0f );
	int floorDraw = img_CreateDraw( whiteImg, 1, floorPos, floorPos, 0 );
	img_SetDrawSize( floorDraw, floorSize, floorSize );
	img_SetDrawColor( floorDraw, floorClr, floorClr );

	drawPlayer( );
	drawEnemies( );

	gsm_Draw( &mainStateMachine );

#if 0
	collision_DebugDrawing( &( player.bodyCollider ), 1, CLR_GREEN );
	collision_DebugDrawing( &( player.damageCollider ), 1, CLR_BLUE );
	if( playerInfos[player.state].weaponActive ) {
		collision_DebugDrawing( &( player.weaponCollider ), 1, CLR_RED );
	}
	drawExplosionColliders( );
#endif

#if 0
	collision_CollectionDebugDrawing( enemyColliderCollection, 1, CLR_RED );
#endif
}

static void otonMainScreen_PhysicsTick( float dt )
{
	gsm_PhysicsTick( &mainStateMachine, dt );
	physicsTick_BG( dt );
}

GameState otonMainScreenState = { otonMainScreen_Enter, otonMainScreen_Exit, otonMainScreen_ProcessEvents,
	otonMainScreen_Process, otonMainScreen_Draw, otonMainScreen_PhysicsTick };