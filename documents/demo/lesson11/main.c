#include "PainterEngine.h"

PX_Object* Previous, * Next, * Image;
px_texture my_texture[10];//���ͼƬ������
px_int index = 0;//��ǰͼƬ������

PX_OBJECT_EVENT_FUNCTION(OnButtonPreClick)
{
    index--;
	if(index < 0)
	{
		index = 9;
	}
	PX_Object_ImageSetTexture(Image, &my_texture[index]);//����ͼƬ
}

PX_OBJECT_EVENT_FUNCTION(OnButtonNextClick)
{
	index++;
	if(index > 9)
	{
		index = 0;
	}
	PX_Object_ImageSetTexture(Image, &my_texture[index]);
}

int main()
{
    px_int i;
    PainterEngine_Initialize(512, 560);//��ʼ��
    for(i=0;i<10;i++)
	{
        px_char path[256];
        PX_sprintf1(path,256, "assets/%1.png", PX_STRINGFORMAT_INT(i+1));
		if(!PX_LoadTextureFromFile(mp_static, &my_texture[i],path))//����ͼƬ
		{
            //����ʧ��
            printf("����ʧ��");
			return 0;
		}
	}
    PainterEngine_LoadFontModule("assets/font.ttf", PX_FONTMODULE_CODEPAGE_GBK, 20);//��������
    Image = PX_Object_ImageCreate(mp, root, 0, 0, 512, 512, my_texture);//����ͼƬ����,Ĭ����ʾ��һ��ͼƬ
    Previous= PX_Object_PushButtonCreate(mp, root, 0, 512, 256, 48, "��һ��",PainterEngine_GetFontModule());//������ť����
    Next = PX_Object_PushButtonCreate(mp, root, 256, 512, 256, 48, "��һ��", PainterEngine_GetFontModule());//������ť����
	PX_ObjectRegisterEvent(Previous, PX_OBJECT_EVENT_EXECUTE, OnButtonPreClick, PX_NULL);//ע�ᰴť�¼�
	PX_ObjectRegisterEvent(Next, PX_OBJECT_EVENT_EXECUTE, OnButtonNextClick, PX_NULL);//ע�ᰴť�¼�
    return 1;
}