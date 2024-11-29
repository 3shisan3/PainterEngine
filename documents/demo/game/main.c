#include "PainterEngine.h"

#define PX_OBJECT_TYPE_FOX		24103001
#define PX_OBJECT_TYPE_HAMMER	24103002
#define PX_OBJECT_TYPE_CLOCK	24103003

PX_FontModule score_fm;
PX_Object* scorePanel;
PX_Object* game,*startgame,*gameclock;

typedef enum
{
	PX_OBJECT_FOX_STATE_IDLE,//���껹�ڶ���
	PX_OBJECT_FOX_STATE_RASING,//������������
	PX_OBJECT_FOX_STATE_TAUNT,//�����ڳ���
	PX_OBJECT_FOX_STATE_ESCAPE,//��������
	PX_OBJECT_FOX_STATE_BEAT,//���걻��
	PX_OBJECT_FOX_STATE_HURT,//�������˺�����
}PX_OBJECT_FOX_STATE;

typedef struct
{
	PX_OBJECT_FOX_STATE state;//����״̬
	px_dword elapsed;//״̬����ʱ��
	px_float texture_render_offset;//������Ⱦƫ��
	px_dword gen_rand_time;//�������ʱ��
	px_float rasing_down_speed;//�����ٶ�
	px_texture render_target;//��ȾĿ��
	px_texture* pcurrent_display_texture;//��ǰ��ʾ������
	px_texture* ptexture_mask;//����
}PX_Object_Fox;

typedef struct
{
	px_texture ham01;//��������1,û�а���
	px_texture ham02;//��������2,����
	px_bool bHit;//�Ƿ���
}PX_Object_Hammer;

typedef struct
{
	PX_Animation animation;//����
	px_dword time;//����ʱʱ��
	px_dword elapsed;//����ʱ��ʼ���Ѿ���ȥ��ʱ��
}PX_Object_Clock;


PX_OBJECT_UPDATE_FUNCTION(PX_Object_ClockUpdate)
{
	PX_Object_Clock* clock = PX_ObjectGetDescByType(pObject, PX_OBJECT_TYPE_CLOCK);
	clock->elapsed += elapsed;
	if (clock->elapsed >= clock->time)
	{
		clock->elapsed = 0;
		PX_ObjectPostEvent(game, PX_OBJECT_BUILD_EVENT(PX_OBJECT_EVENT_RESET));//���ú���״̬,��game�����������¼�
		game->Visible = PX_FALSE;
		game->Enabled = PX_FALSE;
		startgame->Visible = PX_TRUE;
		pObject->Visible = PX_FALSE;
		pObject->Enabled = PX_FALSE;
	}

}

PX_OBJECT_RENDER_FUNCTION(PX_Object_ClockRender)
{
	PX_Object_Clock* clock = PX_ObjectGetDescByType(pObject, PX_OBJECT_TYPE_CLOCK);
	PX_AnimationUpdate(&clock->animation, elapsed);//���¶���
	PX_AnimationRender(psurface, &clock->animation, (px_int)pObject->x, (px_int)pObject->y, PX_ALIGN_CENTER, PX_NULL);//���ƶ���
	//draw ring
	PX_GeoDrawCircle(psurface, (px_int)pObject->x, (px_int)pObject->y, 38, 8, PX_COLOR_BLACK);//���Ƶ���ʱ���߿�
	PX_GeoDrawRing(psurface, (px_int)pObject->x, (px_int)pObject->y, 36, 6, PX_COLOR(128,192,255,32), -90, -90 + (px_int)(360 * (1 - clock->elapsed * 1.0f / clock->time)));//���Ƶ���ʱ��
}

PX_OBJECT_FREE_FUNCTION(PX_Object_ClockFree)
{
	PX_Object_Clock* clock = PX_ObjectGetDescByType(pObject, PX_OBJECT_TYPE_CLOCK);
	PX_AnimationFree(&clock->animation);
}

px_void PX_Object_ClockBegin(PX_Object* pClock, px_dword time)//��ʼ����ʱ
{
	PX_Object_Clock* clock = PX_ObjectGetDescByType(pClock, PX_OBJECT_TYPE_CLOCK);
	clock->time = time;
	pClock->Visible = PX_TRUE;
	pClock->Enabled = PX_TRUE;
}

PX_Object* PX_Object_ClockCreate(px_memorypool* mp, PX_Object* parent, px_float x, px_float y)
{
	PX_Object_Clock* clock;
	PX_Object* pObject = PX_ObjectCreateEx(mp, parent, x, y, 0, 0, 0, 0, PX_OBJECT_TYPE_CLOCK, PX_Object_ClockUpdate, PX_Object_ClockRender, PX_Object_ClockFree, 0, sizeof(PX_Object_Clock));
	clock = PX_ObjectGetDescByType(pObject, PX_OBJECT_TYPE_CLOCK);
	clock->time = 0;
	clock->elapsed = 0;
	if (!PX_AnimationCreate(&clock->animation, PX_ResourceLibraryGetAnimationLibrary(PainterEngine_GetResourceLibrary(), "song")))//����Դ�������л�ȡ����
	{
		PX_ObjectDelete(pObject);
		return PX_NULL;
	}
	pObject->Enabled = PX_FALSE;
	pObject->Visible = PX_FALSE;
	return pObject;
}

PX_OBJECT_UPDATE_FUNCTION(PX_Object_FoxOnUpdate)
{
	PX_Object_Fox* pfox=PX_ObjectGetDescByType(pObject,PX_OBJECT_TYPE_FOX);
	switch (pfox->state)
	{
		case PX_OBJECT_FOX_STATE_IDLE:
		{
			if (pfox->gen_rand_time ==0)
			{
				pfox->gen_rand_time = PX_rand() % 3000 + 1000;//�����ڶ����ʱ��,ʱ�䵽�˾�������
			}
			else
			{
				if (pfox->gen_rand_time <elapsed)//ʱ�䵽��
				{
					//����
					pfox->state = PX_OBJECT_FOX_STATE_RASING;
					pfox->elapsed = 0;
					pfox->gen_rand_time = 0;
					pfox->texture_render_offset = pObject->Height;
					//�ı�����
					pfox->pcurrent_display_texture= PX_ResourceLibraryGetTexture(PainterEngine_GetResourceLibrary(), "fox_rasing");
				}
				else
				{
					pfox->gen_rand_time -= elapsed;
				}
			}
		}
		break;
		case PX_OBJECT_FOX_STATE_RASING://��������
		{
			pfox->elapsed += elapsed;
			//��������ƫ����
			pfox->texture_render_offset -= pfox->rasing_down_speed * elapsed / 1000;
			if (pfox->texture_render_offset <= 0)
			{
				pfox->texture_render_offset = 0;
				pfox->state = PX_OBJECT_FOX_STATE_TAUNT;//����󳰷�
				pfox->elapsed = 0;
			}
		}
		break;
		case PX_OBJECT_FOX_STATE_TAUNT://���곰��
		{
			pfox->elapsed += elapsed;
			if (pfox->elapsed>600&& pfox->elapsed <1500)//����ʱ��
			{
				pfox->pcurrent_display_texture = PX_ResourceLibraryGetTexture(PainterEngine_GetResourceLibrary(), "fox_taunt");//��������
			}
			else if (pfox->elapsed>1500)//�������
			{
				pfox->texture_render_offset = 0;
				pfox->state = PX_OBJECT_FOX_STATE_ESCAPE;//����
				pfox->pcurrent_display_texture = PX_ResourceLibraryGetTexture(PainterEngine_GetResourceLibrary(), "fox_escape");//��������
				pfox->elapsed = 0;
			}
		}
		break;
		case PX_OBJECT_FOX_STATE_BEAT://���걻��
		{
			pfox->elapsed += elapsed;
			if (pfox->elapsed>800)
			{
				pfox->pcurrent_display_texture = PX_ResourceLibraryGetTexture(PainterEngine_GetResourceLibrary(), "fox_hurt");//��������
				pfox->state = PX_OBJECT_FOX_STATE_ESCAPE;//����
			}
		}
		break;
		case PX_OBJECT_FOX_STATE_ESCAPE:
		{
			pfox->elapsed += elapsed;
			pfox->texture_render_offset+=pfox->rasing_down_speed * elapsed / 1000;
			if (pfox->texture_render_offset >= pObject->Height)
			{
				pfox->texture_render_offset = pObject->Height;
				pfox->state = PX_OBJECT_FOX_STATE_IDLE;//���ܽ���
				pfox->elapsed = 0;//����ʱ��
				pfox->pcurrent_display_texture = PX_NULL;
			}
		}
		break;
	default:
		break;
	}
}

PX_OBJECT_RENDER_FUNCTION(PX_Object_FoxOnRender)
{
	PX_Object_Fox* pfox = PX_ObjectGetDescByType(pObject, PX_OBJECT_TYPE_FOX);
	px_float x,y,width,height;
	PX_OBJECT_INHERIT_CODE(pObject,x,y,width,height);
	PX_TextureClearAll(&pfox->render_target, PX_COLOR_NONE);//�����ȾĿ��
	if (pfox->pcurrent_display_texture)
	{
		PX_TextureRender(&pfox->render_target, pfox->pcurrent_display_texture, (px_int)pfox->render_target.width/2, (px_int)pfox->texture_render_offset, PX_ALIGN_MIDTOP, PX_NULL);//��Ⱦ����
	}
	PX_TextureRenderMask(psurface, pfox->ptexture_mask, &pfox->render_target, (px_int)x, (px_int)y, PX_ALIGN_MIDBOTTOM, PX_NULL);//��������ʽ��������
}

PX_OBJECT_FREE_FUNCTION(PX_Object_FoxFree)
{
	PX_Object_Fox* pfox = PX_ObjectGetDescByType(pObject, PX_OBJECT_TYPE_FOX);
	PX_TextureFree(&pfox->render_target);
}

PX_OBJECT_EVENT_FUNCTION(PX_Object_FoxOnClick)//���걻���
{
	PX_Object_Fox* pfox = PX_ObjectGetDescByType(pObject, PX_OBJECT_TYPE_FOX);
	if (pfox->state == PX_OBJECT_FOX_STATE_TAUNT|| pfox->state == PX_OBJECT_FOX_STATE_RASING)//���곰���������ʱ�����Ч
	{
		if (PX_ObjectIsCursorInRegionAlign(pObject, e, PX_ALIGN_MIDBOTTOM))//�����Ч����
		{
			px_int x= (px_int)PX_Object_Event_GetCursorX(e);
			px_int y= (px_int)PX_Object_Event_GetCursorY(e);
			x=(px_int)(x-(pObject->x-pObject->Width/2));
			y= (px_int)(y-(pObject->y - pObject->Height));
			if (x>32&&y<128)
			{
				pfox->pcurrent_display_texture = PX_ResourceLibraryGetTexture(PainterEngine_GetResourceLibrary(), "fox_beat");
				pfox->state = PX_OBJECT_FOX_STATE_BEAT;
				pfox->elapsed = 0;
				PX_Object_ScorePanelAddScore(scorePanel, 100);
			}
			
		}
	}
}

PX_OBJECT_EVENT_FUNCTION(PX_Object_FoxOnReset)
{
	PX_Object_Fox* pfox = PX_ObjectGetDescByType(pObject, PX_OBJECT_TYPE_FOX);
	pfox->state = PX_OBJECT_FOX_STATE_IDLE;
	pfox->elapsed = 0;
	pfox->texture_render_offset = pObject->Height;
	pfox->gen_rand_time = 0;
	pfox->pcurrent_display_texture = PX_NULL;

}

PX_Object *PX_Object_FoxCreate(px_memorypool *mp,PX_Object *parent,px_float x,px_float y)
{
	PX_Object_Fox* pfox;
	px_texture *ptexture=PX_ResourceLibraryGetTexture(PainterEngine_GetResourceLibrary(),"fox_rasing");//����Դ�������л�ȡ����
	PX_Object* pObject = PX_ObjectCreateEx(mp, parent, x, y, 0, ptexture->width*1.f, ptexture->height*1.f, 0, PX_OBJECT_TYPE_FOX, PX_Object_FoxOnUpdate, PX_Object_FoxOnRender, PX_Object_FoxFree, 0, sizeof(PX_Object_Fox));
	pfox=PX_ObjectGetDescByType(pObject,PX_OBJECT_TYPE_FOX);
	pfox->state= PX_OBJECT_FOX_STATE_IDLE;//����״̬
	pfox->rasing_down_speed = 512;//�����ٶ�
	pfox->ptexture_mask = PX_ResourceLibraryGetTexture(PainterEngine_GetResourceLibrary(), "fox_mask");//����
	if(!PX_TextureCreate(mp,&pfox->render_target,ptexture->width,ptexture->height))
	{
		PX_ObjectDelete(pObject);
		return 0;
	}
	PX_ObjectRegisterEvent(pObject,PX_OBJECT_EVENT_CURSORDOWN,PX_Object_FoxOnClick,0);//ע�����¼�
	PX_ObjectRegisterEvent(pObject,PX_OBJECT_EVENT_RESET,PX_Object_FoxOnReset,0);//ע�������¼�
	return pObject;
}

PX_OBJECT_RENDER_FUNCTION(PX_Object_HammerRender)//������Ⱦ
{
	PX_Object_Hammer* phammer = PX_ObjectGetDescByType(pObject, PX_OBJECT_TYPE_HAMMER);
	px_float x, y, width, height;
	PX_OBJECT_INHERIT_CODE(pObject, x, y, width, height);
	if (phammer->bHit)
	{
		PX_TextureRender(psurface, &phammer->ham02, (px_int)x, (px_int)y, PX_ALIGN_CENTER, PX_NULL);//����
	}
	else
	{
		PX_TextureRender(psurface, &phammer->ham01, (px_int)x, (px_int)y, PX_ALIGN_CENTER, PX_NULL);//δ����
	}
	
}

PX_OBJECT_FREE_FUNCTION(PX_Object_HammerFree)
{
	PX_Object_Hammer* phammer = PX_ObjectGetDescByType(pObject, PX_OBJECT_TYPE_HAMMER);
	PX_TextureFree(&phammer->ham01);
	PX_TextureFree(&phammer->ham02);
}

PX_OBJECT_EVENT_FUNCTION(PX_Object_HammerOnMove)
{
	pObject->x=PX_Object_Event_GetCursorX(e);//���Ӹ�������ƶ�
	pObject->y=PX_Object_Event_GetCursorY(e);
}

PX_OBJECT_EVENT_FUNCTION(PX_Object_HammerOnCursorDown)
{
	PX_Object_Hammer* phammer = PX_ObjectGetDescByType(pObject, PX_OBJECT_TYPE_HAMMER);
	phammer->bHit = PX_TRUE;//����
}

PX_OBJECT_EVENT_FUNCTION(PX_Object_HammerOnCursorUp)
{
	PX_Object_Hammer* phammer = PX_ObjectGetDescByType(pObject, PX_OBJECT_TYPE_HAMMER);
	phammer->bHit = PX_FALSE;//̧��
}

PX_Object* PX_Object_HammerCreate(px_memorypool* mp, PX_Object* parent)
{
	PX_Object_Hammer* phammer;
	PX_Object* pObject = PX_ObjectCreateEx(mp, parent, 0, 0, 0, 0, 0, 0, PX_OBJECT_TYPE_HAMMER, 0, PX_Object_HammerRender, PX_Object_HammerFree, 0, sizeof(PX_Object_Hammer));
	phammer = PX_ObjectGetDescByType(pObject, PX_OBJECT_TYPE_HAMMER);
	phammer->bHit = PX_FALSE;
	if (!PX_LoadTextureFromFile(mp_static,&phammer->ham01, "assets/ham1.png")) return PX_NULL;
	if (!PX_LoadTextureFromFile(mp_static,&phammer->ham02, "assets/ham2.png")) return PX_NULL;
	PX_ObjectRegisterEvent(pObject, PX_OBJECT_EVENT_CURSORMOVE, PX_Object_HammerOnMove, PX_NULL);//ע���ƶ��¼�
	PX_ObjectRegisterEvent(pObject, PX_OBJECT_EVENT_CURSORDRAG, PX_Object_HammerOnMove, PX_NULL);//ע����ק�¼�
	PX_ObjectRegisterEvent(pObject, PX_OBJECT_EVENT_CURSORDOWN, PX_Object_HammerOnCursorDown, PX_NULL);//ע�ᰴ���¼�
	PX_ObjectRegisterEvent(pObject, PX_OBJECT_EVENT_CURSORDOWN, PX_Object_HammerOnMove, PX_NULL);//ע�ᰴ���¼�
	PX_ObjectRegisterEvent(pObject, PX_OBJECT_EVENT_CURSORUP, PX_Object_HammerOnCursorUp, PX_NULL);//ע��̧���¼�

	return pObject;
}

PX_OBJECT_EVENT_FUNCTION(PX_Object_StartGameOnClick)
{
	game->Visible = PX_TRUE;
	startgame->Visible = PX_FALSE;
	game->Enabled = PX_TRUE;
	PX_Object_ScorePanelSetScore(scorePanel, 0);
	PX_Object_ClockBegin(gameclock, 30000);//��ʼ��Ϸ,��Ϸʱ��30��
}


px_int main()
{
	px_int i;
	PainterEngine_Initialize(800, 480);
	PX_FontModuleInitialize(mp_static,&score_fm);
	PX_FontModuleSetCodepage(&score_fm, PX_FONTMODULE_CODEPAGE_GBK);
	if (!PX_LoadTextureToResource(PainterEngine_GetResourceLibrary(), "assets/rasing.png", "fox_rasing")) return 0;
	if (!PX_LoadTextureToResource(PainterEngine_GetResourceLibrary(), "assets/taunt.png", "fox_taunt")) return 0;
	if (!PX_LoadTextureToResource(PainterEngine_GetResourceLibrary(), "assets/escape.png", "fox_escape")) return 0;
	if (!PX_LoadTextureToResource(PainterEngine_GetResourceLibrary(), "assets/beat.png", "fox_beat")) return 0;
	if (!PX_LoadTextureToResource(PainterEngine_GetResourceLibrary(), "assets/hurt.png", "fox_hurt")) return 0;
	if (!PX_LoadTextureToResource(PainterEngine_GetResourceLibrary(), "assets/mask.png", "fox_mask")) return 0;
	if (!PX_LoadTextureToResource(PainterEngine_GetResourceLibrary(), "assets/background.png", "background")) return 0;
	if (!PX_LoadAnimationToResource(PainterEngine_GetResourceLibrary(), "assets/song.2dx", "song"))return 0;
	PainterEngine_SetBackgroundTexture(PX_ResourceLibraryGetTexture(PainterEngine_GetResourceLibrary(), "background"));
	for (i = 0; i <= 9; i++)
	{
		px_texture tex;
		px_char path[64];
		PX_sprintf1(path,64, "assets/%1.png", PX_STRINGFORMAT_INT(i));
		if (PX_LoadTextureFromFile(mp,&tex,path))
		{
			PX_FontModuleAddNewTextureCharacter(&score_fm, '0' + i, &tex);
		}
		PX_TextureFree(&tex);
	}
	
	startgame = PX_Object_PushButtonCreate(mp, root, 300, 200, 200, 90, "Start Game", 0);
	startgame->Visible = PX_TRUE;
	PX_Object_PushButtonSetBackgroundColor(startgame, PX_COLOR(96, 255, 255, 255));
	PX_Object_PushButtonSetPushColor(startgame, PX_COLOR(224, 255, 255, 255));
	PX_Object_PushButtonSetCursorColor(startgame, PX_COLOR(168, 255, 255, 255));
	PX_ObjectRegisterEvent(startgame, PX_OBJECT_EVENT_EXECUTE, PX_Object_StartGameOnClick, 0);

	
	
	game=PX_ObjectCreate(mp, root, 0, 0, 0, 0, 0, 0);
	PX_Object_FoxCreate(mp, game, 173, 326);
	PX_Object_FoxCreate(mp, game, 401, 326);
	PX_Object_FoxCreate(mp, game, 636, 326);
	PX_Object_FoxCreate(mp, game, 173, 476);
	PX_Object_FoxCreate(mp, game, 401, 476);
	PX_Object_FoxCreate(mp, game, 636, 476);
	game->Visible=PX_FALSE;
	game->Enabled=PX_FALSE;

	
	PX_Object_HammerCreate(mp, root);
	scorePanel = PX_Object_ScorePanelCreate(mp, root, 400, 60, &score_fm, 100);

	gameclock=PX_Object_ClockCreate(mp,root,680,60);
	
	return PX_TRUE;
}