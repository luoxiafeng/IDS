#include <common.h>
#include <asm/io.h>
#include <ids/ids.h>
#include <dmmu.h>

static struct reg_check reg[] = {
	REG_CHECK_INIT(OVCDCR,			0x00407FFF),
	REG_CHECK_INIT(OVCPCR,			0x0000803F),
	REG_CHECK_INIT(OVCBKCOLOR,		0x00FFFFFF),
	REG_CHECK_INIT(OVCWPR,			0x00FFFFFF),
	REG_CHECK_INIT(OVCW0CR,			0x0007FF7F),
	REG_CHECK_INIT(OVCW0PCAR,		0x0FFF0FFF),
	REG_CHECK_INIT(OVCW0PCBR,		0x0FFF0FFF),
	REG_CHECK_INIT(OVCW0VSSR,		0x77FFFFFF),
	REG_CHECK_INIT(OVCW0CMR,		0x01FFFFFF),
	REG_CHECK_INIT(OVCW0B0SAR,		0xFFFFFFFF),
	REG_CHECK_INIT(OVCW0B1SAR,		0xFFFFFFFF),
	REG_CHECK_INIT(OVCW0B2SAR,		0xFFFFFFFF),
	REG_CHECK_INIT(OVCW0B3SAR,		0xFFFFFFFF),
	REG_CHECK_INIT(OVCW1CR,			0x0007FFFF),
	REG_CHECK_INIT(OVCW1PCAR,		0x0FFF0FFF),
	REG_CHECK_INIT(OVCW1PCBR,		0x0FFF0FFF),
	REG_CHECK_INIT(OVCW1PCCR,		0x00FFFFFF),
	REG_CHECK_INIT(OVCW1VSSR,		0x77FFFFFF),
	REG_CHECK_INIT(OVCW1CKCR,		0x07FFFFFF),
	REG_CHECK_INIT(OVCW1CKR,		0x00FFFFFF),
	REG_CHECK_INIT(OVCW1CMR,		0x01FFFFFF),
	REG_CHECK_INIT(OVCW1B0SAR,		0xFFFFFFFF),
	REG_CHECK_INIT(OVCW1B1SAR,		0xFFFFFFFF),
	REG_CHECK_INIT(OVCW1B2SAR,		0xFFFFFFFF),
	REG_CHECK_INIT(OVCW1B3SAR,		0xFFFFFFFF),
	REG_CHECK_INIT(OVCBRB0SAR,		0xFFFFFFFF),
	REG_CHECK_INIT(OVCBRB1SAR,      0xFFFFFFFF),
	REG_CHECK_INIT(OVCBRB2SAR,      0xFFFFFFFF),	
	REG_CHECK_INIT(OVCBRB3SAR,      0xFFFFFFFF),
	REG_CHECK_INIT(OVCOMC,			0x811FFFFF),
	REG_CHECK_INIT(OVCOEF11,		0x000007FF),
	REG_CHECK_INIT(OVCOEF12,		0x000007FF),
	REG_CHECK_INIT(CICOEF13,		0x000007FF),
	REG_CHECK_INIT(OVCOEF21,		0x000007FF),
	REG_CHECK_INIT(OVCOEF22,		0x000007FF),
	REG_CHECK_INIT(OVCOEF23,		0x000007FF),
	REG_CHECK_INIT(OVCOEF31,		0x000007FF),
	REG_CHECK_INIT(OVCOEF32,		0x000007FF),
	REG_CHECK_INIT(OVCOEF33,		0x000007FF),
	REG_CHECK_INIT(OVCPSCCR,		0x1FFFDFFF),
	REG_CHECK_INIT(OVCPSCPCR,		0x07FF07FF),
	REG_CHECK_INIT(OVCPSVSSR,		0x77FFFFFF),
	REG_CHECK_INIT(OVCPSB0SAR,		0xFFFFFFFF),
	REG_CHECK_INIT(OVCPSB1SAR,		0xFFFFFFFF),
	REG_CHECK_INIT(OVCPSB2SAR,		0xFFFFFFFF),
	REG_CHECK_INIT(OVCPSB3SAR,		0xFFFFFFFF),
	REG_CHECK_INIT(DIT_FSR,				0x0FFF0FFF),
	REG_CHECK_INIT(DIT_DCR,				0x00000FFD),
	REG_CHECK_INIT(DIT_DMCO(0),			0x1F1F1F1F),
	REG_CHECK_INIT(DIT_DMCO(1),         0x1F1F1F1F),
	REG_CHECK_INIT(DIT_DMCO(2),         0x1F1F1F1F),
	REG_CHECK_INIT(DIT_DMCO(3),         0x1F1F1F1F),
	REG_CHECK_INIT(DIT_DMCO(4),         0x1F1F1F1F),
	REG_CHECK_INIT(DIT_DMCO(5),         0x1F1F1F1F),
	REG_CHECK_INIT(DIT_DMCO(6),         0x1F1F1F1F),
	REG_CHECK_INIT(DIT_DMCO(7),         0x1F1F1F1F),
};

int osd_reg_check(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(reg); i++)
		reg[i].addr += reg_base;

	if (common_reg_check(reg, ARRAY_SIZE(reg), 0xffffffff) || common_reg_check(reg, ARRAY_SIZE(reg), 0))
		return -1;

	return 0;
}

/*
*(1)这个函数跟dma没有一毛钱关系。
*(2)就是设置了OSD 控制器内部的buffer的使用。
*(3)总共有4个buffer，至少使用两个buffer。当有多个buffer的时候，可以通过配置寄存器决定使用哪一个buffer。
*/
void osd_set_dma_mode(int nr, struct osd_buf_mode *mode)
{
	uint32_t val, offset = OVCW0CR;
	
	if (nr)
		offset = OVCW1CR;

	val = readl(reg_base + offset);
	val &= ~(0x1f << OVCWxCR_BUFNUM);
	val |= (((mode->bufsel & 0x3) << OVCWxCR_BUFSEL) | //buffer选择
		   ((mode->bufauto & 0x1) << OVCWxCR_BUFAUTOEN) | //buffer控制标志位
		   ((mode->bufnum & 0x3) << OVCWxCR_BUFNUM));   //选择开启的buffer数量
	writel(val, reg_base + offset);
}
/*
*(1)osd其实就是由两个窗口组成的。
*(2)win0和win2.osd的使能说白了就是对应的窗口的使能。
*/
static void osd_enable_ctrl(int nr, int enable)
{
	uint32_t val, offset = OVCW0CR;
	
	if (nr)
		offset = OVCW1CR;
	val = readl(reg_base + offset);
	if (enable)
		val |= (0x1 << OVCWxCR_ENWIN); //win使能
	else
		val &= ~(0x1 << OVCWxCR_ENWIN);//win不使能
	writel(val, reg_base + offset);
}

static void osd_palette_ctrl(int enable)
{
	writel(enable << OVCPCR_UPDATE_PAL, reg_base + OVCPCR);
}

void osd_set_bg_color(uint32_t val)
{
	writel(val, reg_base + OVCBKCOLOR);
}

void osd_set_scale_xy(int x, int y)
{
	uint32_t val;

	val = ((x - 1) << OVCWPR_OSDWinX) |
		  ((y - 1) << OVCWPR_OSDWinY);
	writel(val, reg_base + OVCWPR);
}
/*
*(1)设置OSD图像的左上角x坐标和y坐标
*(2)配置的是OSD的窗口0位置寄存器A
*/
static void osd_set_start_xy(int nr, int x, int y)
{
	uint32_t val, offset = OVCW0PCAR;
	
	if (nr)
		offset = OVCW1PCAR;

	val = (x << OVCWxPCAR_LeftTopX) |
		  (y << OVCWxPCAR_LeftTopY);
	writel(val, reg_base + offset);
}
/*
*(1)配置的是窗口0位置寄存器B
*(2)OSD图像的右下角坐标x和y的值
*/
void osd_set_end_xy(int nr, int x, int y)
{
	uint32_t val, offset = OVCW0PCBR;
	
	if (nr)
		offset = OVCW1PCBR;
	val = ((x - 1) << OVCWxPCBR_RightBotX) |
		  ((y - 1) << OVCWxPCBR_RightBotY);
	writel(val, reg_base + offset);
}
/*
*(1)coord是windows api中的结构体，表示一个字符在控制台上的坐标。
*(2)通过设置OSD图像win0的左上角坐标（x，y）和右下角坐标(x,y)，来确定osd的win0的位置
*/
void osd_set_coord(int nr, struct osd_xy *coord)
{
	osd_set_start_xy(nr, coord->leftx, coord->lefty);
	osd_set_end_xy(nr, coord->rightx, coord->righty);	
}
/*
*(1)OSD窗口0，虚拟屏宽度设置寄存器
*(2)虚拟屏的宽度，单位是像素
*/
void osd_set_vm_width(int nr, int val)
{
	uint32_t tmp, offset = OVCW0VSSR;

	if (nr)
		offset = OVCW1VSSR;
	tmp = val << OVCWVSSR_VW_WIDTH;
	writel(tmp, reg_base + offset);
}

void osd_set_yuvtorgb(struct osd_yuv *yuv)
{
	uint32_t val;
	int i;
	
	val = readl(reg_base + OVCOMC);
	val &= ~(0x1 << 31);
	val |= (yuv->enable << 31);
	writel(val, reg_base + OVCOMC);
	if (yuv->enable) {
		val = readl(reg_base + OVCOMC);
		val &= ~(0x1f << 0);
		val &= ~(0x1f << 8);
		val |= (yuv->out_oft_a) & 0x1f;
		val |= ((yuv->out_oft_b & 0x1f) << 8);
		writel(val, reg_base + OVCOMC);
		for (i = 0; i < 9; i++)
			writel(yuv->out_coef[i] & 0x7ff, OVCOEF11 + 4 * i + reg_base);
	}
}
/*
*(1)这个使能之后，被覆盖的区域的图像数据不再被读取。
*(2)并且遮盖的颜色将替换原图。这里给了一个24bit的颜色，可以用这个颜色来替换【区域】中的原图。
*(3)既然是24bit的，就是一个真彩色的颜色值。
*/
void osd_colormap_ctrl(int nr, int enable, int color)
{
	uint32_t val, offset = OVCW0CMR;
	
	if (nr)
		offset = OVCW1CMR;

	val = (enable <<  OVCWCMRx_ENABLE) |
		  (color << OVCWCMRx_COLOR);
	writel(val, reg_base + offset);
}

void osd_set_changerb(int nr, int enable)
{       
        uint32_t offset = OVCW0CR;
        uint32_t val;
        
        if (nr)
                offset = OVCW1CR;
        
        val = readl(reg_base + offset);
        if (enable)
                val |= (0x1 << OVCWxCR_RBEXG);
        else
                val &= ~(0x1 << OVCWxCR_RBEXG);
        writel(val, reg_base + offset);
}

/*
*(1)牛头不对马嘴。这个函数的意思是设置framebuffer的偏移。
*(2)我们已经知道，ids控制器中最多有4个buffer，每个buffer中的有效数据可以通过寄存器来配置。
*   说白了，就是可以任意指定buffer的有效数据。
*/
void osd_set_dma_channel(int nr, int num, struct dma_buffer *dma)
{
	uint32_t offset1 = OVCW0B0SAR;
	int i;
	
	if (num > 4)
		num = 4;

	if (num < 1 || num > 4) {
		ids_err("Window dma addr only support [1, 4]\n");
		return;
	}
	
	if (nr)
		offset1 = OVCW1B0SAR;
        /*
        *(1)我们可以看出来，dma[i].vir_addr指向的是malloc动态分配的内存。
        *(2)我们把这个虚拟地址，写到控制器的buffer寄存器，也就是说，控制器内部是没有buffer的，只是有一个4字节的寄存器
        *   需要我们分配内存，然后把内存的首地址告诉这个寄存器。这个寄存器就知道buffer在哪里了。
        */
	for (i = 0; i < num;i++)
		writel((uint32_t)(dma[i].vir_addr), reg_base + offset1 + i * 4);
}

void osd_set_yuv_dma(int nr, int num, struct dma_buffer *dma)
{
	uint32_t offset1 = OVCW0B0SAR;
	int i;

	if (nr)
		offset1 = OVCW1B0SAR;

	for (i = 0; i < num;i++) {
		writel((uint32_t)(dma[2 * i].vir_addr), reg_base + offset1 + i * 4);
		writel((uint32_t)(dma[2 * i + 1].vir_addr), reg_base + 0x1300 + i * 4);
	}
}

/*
*(1)直接往OSD窗口0控制寄存器中的bpp控制位写
*/
void osd_set_bppmode(int nr, int mode)
{
	uint32_t val, offset = OVCW0CR;
	
	if (nr)
		offset = OVCW1CR;
	val = readl(reg_base + offset);
	val &= ~(0x1f << OVCWxCR_BPPMODE);
	val |= (mode << OVCWxCR_BPPMODE);
	writel(val, reg_base + offset);
}
/*
*(1)只有win1才需要设置colorkey的值。
*(2)如果是win0，则不需要设置colorkey。
*/
void osd_set_colorkey(int nr, struct osd_colorkey *colorkey)
{
	uint32_t val;

	if (nr < 1)
		return;

	val = readl(reg_base + OVCW1CKCR);
	val = ((colorkey->enable << OVCWxCKCR_KEYEN) |  //color key 使能控制
		   (colorkey->enableBlend << OVCWxCKCR_KEYBLEN) | //color key 混合控制：0-关闭、1-使能。在非key区域利用alpha0混合；在key区域，利用alpha1混合
		   (colorkey->dircon << OVCWxCKCR_DIRCON) | //方向控制。与color-key一样的就不显示。选择方向分为：前景色、背景色
		   (colorkey->compkey << OVCWxCKCR_COMPKEY));
	writel(val, reg_base + OVCW1CKCR);
	
	writel(colorkey->colval, reg_base + OVCW1CKR);
}
/*
*(1)只有win1才需要设置alpha的值。
*(2)如果是win0，则不需要设置alpha。
*/
void osd_set_alpha(int nr, struct osd_alpha *alpha)
{
	uint32_t val;
	
	if (nr < 1)
		return;
	/*
	*(1)窗口1控制寄存器
	*(2)设置alpha的值；选择混合种类：平面混合、像素混合
	*(3)按照杨磊的意思，所谓的像素混合，就是RGB像素值中有alpha分量的值；所谓的平面混合，就是像素数据没有alpha的值，需要
	*   人为给定一个alpha的值。
	*(4)如果需要人为设定alpha的值，则混合种类就应该选择平面混合了。
	*/
	val = readl(reg_base + OVCW1CR);
	val &= ~(0x3 << OVCWxCR_BLD_PIX);
	val |= ((alpha->path << OVCWxCR_ALPHA_SEL) |
			(alpha->blendmode << OVCWxCR_BLD_PIX));
	writel(val, reg_base + OVCW1CR);
	/*
	*(1)这是一个32bit的寄存器，有效位数为24bit。每个占据4bit，总共6个数据。
	*/
	val = ((alpha->alpha0_r << OVCWxPCCR_ALPHA0_R) |
		   (alpha->alpha0_g << OVCWxPCCR_ALPHA0_G) |
		   (alpha->alpha0_b << OVCWxPCCR_ALPHA0_B) |
		   (alpha->alpha1_r << OVCWxPCCR_ALPHA1_R) |
		   (alpha->alpha1_g << OVCWxPCCR_ALPHA1_G) |
		   (alpha->alpha1_b << OVCWxPCCR_ALPHA1_B));
	writel(val, reg_base + OVCW1PCCR);
}

/*
*(1)设置一个byte交换寄存器。1-表示允许交换；0表示不允许交换。
*(2)现在设置的是0.
*/
void osd_set_pixel_swap(int nr, struct osd_swap *swap)
{
	uint32_t val, offset = OVCW0CR;
	
	if (nr)
		offset = OVCW1CR;
	
	val = readl(reg_base + offset);
	if (swap->swap == OSD_EXCHANGERB) {
		if (swap->enable)
			val |= (0x1 << OVCWxCR_RBEXG);
		else
			val &= ~(0x1 << OVCWxCR_RBEXG);
	} else {
		if (swap->enable)
			val |= (0x1 << OVCWxCR_HAWSWP(swap->swap));
		else
			val &= ~(0x1 << OVCWxCR_HAWSWP(swap->swap));
	}
	writel(val, reg_base + offset);
}
/*
*(1)设置scaler的模式：在osd之前、在osd之后。
*/
void osd_set_scaler_mode(enum scaler_mode mode)
{
	uint32_t val;

	val = readl(reg_base + OVCDCR);
	val &= ~(0x1 << OVCDCR_ScalerMode);//先把bit为对应的寄存器设置为0
	val |= (mode << OVCDCR_ScalerMode);//把要设置的mode写进寄存器
	writel(val, reg_base + OVCDCR);
}

void osd_set_prescaler_param(struct prescaler_para *para)
{
	uint32_t val;
	int i;

	val = (para->window << 20) | (para->format << 1);
	writel(val, reg_base + OVCPSCCR);
	val = ((para->x - 1)<< 0) | ((para->y - 1) << 16);
	writel(val, reg_base + OVCPSCPCR);
	writel(para->x, reg_base + OVCPSVSSR);
	if (para->format != OSD_IMAGE_YUV_420SP)
		for (i = 0; i < para->dma->dma_num; i++)
			writel((uint32_t)(para->dma->buf[i].vir_addr), reg_base + OVCPSB0SAR + i * 4);
	else {
		for (i = 0; i < para->dma->dma_num / 2; i++) {
			writel((uint32_t)(para->dma->buf[2 * i].vir_addr), reg_base + OVCPSB0SAR + i * 4);
			writel((uint32_t)(para->dma->buf[2 * i + 1].vir_addr), reg_base + OVCPSCB0SAR + i * 4);
		}
	}
}

void osd_prescaler_bppmode(int mode)
{
	int val;

	val = readl(reg_base + OVCPSCCR);
	val &= ~(0x1f << 1);
	val |= ((mode & 0x1f) << 1);
	writel(val, reg_base + OVCPSCCR);
}

void osd_prescalser_cood(int x, int y)
{
	writel((x - 1) | ((y - 1) << 16), reg_base + OVCPSCPCR);
}

void osd_prescaler_enable(int flags)
{
	uint32_t val;

	val = readl(reg_base + OVCPSCCR);
	val &= ~(0x1);	
	val |= flags;
	writel(val, reg_base + OVCPSCCR);
}

void osd_prescaler_dma_mode(int nr, struct osd_buf_mode *mode)
{
	uint32_t val, offset = OVCPSCCR;
	
	val = readl(reg_base + offset);
	val &= ~(0x1f << OVCWxCR_BUFNUM);
	val |= (((mode->bufsel & 0x3) << OVCWxCR_BUFSEL) |
		   ((mode->bufauto & 0x1) << OVCWxCR_BUFAUTOEN) |
		   ((mode->bufnum & 0x3) << OVCWxCR_BUFNUM));
	writel(val, reg_base + offset);
}

#if 0
void osd_set_scaler_input_size(int height, int width)
{
	uint32_t val;

	val = ((height - 1) << SCALER_INPUT_FRAME_VER) |
		  ((width - 1) << SCALER_INPUT_FRAME_HOR);
	writel(val, reg_base + SCLIFSR);
}

void osd_set_scaler_output_size(int height, int width)
{
	uint32_t val;

	val = ((height - 1) << SCALER_OUTPUT_FRAME_VER) |
		  ((width - 1) << SCALER_OUTPUT_FRAME_HOR);
	writel(val, reg_base + SCLOFSR);
}

void osd_set_scaler_output_offset(int height, int width)
{
	uint32_t val;

	val = ((height - 1) << SCALER_OFFSET_FRAME_VER) |
		  ((width - 1) << SCALER_OFFSET_FRAME_HOR);
	writel(val, reg_base + SCLOFOR);
}

void osd_set_height_scaler(struct osd_scaler *scaler)
{
	uint32_t val;

	val = readl(reg_base + SCLCR);
	val &= ~(0xffff << 16);
	val |= ((scaler->roundpolicy << SCALER_CTRL_VROUND) |
			(scaler->enable << SCALER_CTRL_VPASSBY) |
			((scaler->zoomout & 0xf) << SCALER_CTRL_VSCALING_ZOOMOUT) |
			((scaler->zoomin & 0x3ff) << SCALER_CTRL_VSCALING_ZOOMIN));
	writel(val, reg_base + SCLCR);
}

void osd_set_width_scaler(struct osd_scaler *scaler)
{
	uint32_t val;

	val = readl(reg_base + SCLCR);
	val &= ~(0xffff);
	val |= ((scaler->roundpolicy << SCALER_CTRL_HROUND) |
		    (scaler->enable << SCALER_CTRL_HPASSBY) |
			((scaler->zoomout & 0xf) << SCALER_CTRL_HSCALING_ZOOMOUT) |
			((scaler->zoomin & 0x3ff) << SCALER_CTRL_HSCALING_ZOOMIN));
	writel(val, reg_base + SCLCR);
}
#endif

void osd_set_pallete(int nr, struct osd_pallete *pallete)
{
	uint32_t val = 0, offset = OVCW0PAL0;//osd win0的调色板RAM。就是256个4字节的寄存器。
	int i;

	if (nr)
		offset = OVCW1PAL0;//osd win1的调色板RAM。就是256个4字节的寄存器。

	if (!pallete || !pallete->table)
		return;

	if (pallete->tableLength) {
		/*
		*(1)这是ids的控制寄存器。
		*(2)先把这个寄存器的低三个bit设置为0.这三个bit表示调色板的数据格式。
		*(3)然后把我们希望的数据格式，设置到寄存器中。16BIT,18BIT,19BIT,24BIT,25BIT
		*/
		val = readl(reg_base + OVCPCR);
		if (nr) {
			val &= ~(0x7 << OVCPCR_W1PALFM);
			val |= (pallete->format << OVCPCR_W1PALFM);
		} else {
			val &= ~(0x7 << OVCPCR_W0PALFM);
			val |= (pallete->format << OVCPCR_W0PALFM);
		}
		/*
		*(1)设置调色板的开关。这个寄存器就相当于一个开关。打开，则调色板memory可以修改。
		*(2)
		*/
		val |= (0x1 << OVCPCR_UPDATE_PAL);
		writel(val, reg_base + OVCPCR);
		/*
		*(1)之前我们已经介绍过，调色板有256个颜色，每个颜色就是一个数值（0-255）.RGB三个字节，对于4字节的寄存器
		*   绰绰有余
		*/
		for (i = 0; i < pallete->tableLength; i++)
			writel(pallete->table[i], reg_base + offset + 4 * i);
		/*
		*(1)设置调色板的开关。关闭。
		*(2)调色板中的颜色修改好了之后，就关闭。关闭后，调色板的颜色就锁定了，不再改变了。
		*/
		val = readl(reg_base + OVCPCR);
		val &= ~(0x1 << OVCPCR_UPDATE_PAL);
		writel(val, reg_base + OVCPCR);
	}
}

void osd_set_pre_pallete(struct osd_pallete *pallete)
{
	uint32_t val = 0, i;

	if (!pallete || !pallete->table)
		return;

	if (pallete->tableLength) {
		val = readl(reg_base + OVCPSCCR);
		val &= ~(0x7 << 28);
		val |= (pallete->format << 28);
		writel(val, reg_base + OVCPSCCR);
		for (i = 0; i < pallete->tableLength; i++)
			writel(pallete->table[i],  reg_base + 0x1F80 + 4 * i);
	}
}

void osd_set_global_load(int enable)
{
	uint32_t val;

	val = readl(reg_base + OVCDCR);
	val &= ~(0x1 << OVCDCR_LOAD_EN);
	val |= (enable << OVCDCR_LOAD_EN);
	writel(val, reg_base + OVCDCR);
}

void osd_set_output_format(struct osd_output_format *format)
{
	int val;

	val = readl(reg_base + OVCDCR);
	val &= (~0x3);
	val |= ((format->interlace << OVCDCR_Interlace) |
		   (format->iftype << OVCDCR_IfType));
	writel(val, reg_base + OVCDCR);
}

void osd_win_init(struct osd_win_cfg *cfg)
{
	if ((cfg->nr != 0) && (cfg->nr != 1)) {
		ids_err("Not support this windows: %d\n", cfg->nr);
		return;
	}
	/* set dma mode */
	osd_set_dma_mode(cfg->nr, &cfg->buf_mode);
	/* set bpp mode */
	osd_set_bppmode(cfg->nr, cfg->format);
	/* set xy cood */
	osd_set_coord(cfg->nr, &cfg->coord);
	/* vm_width */
	osd_set_vm_width(cfg->nr, cfg->coord.rightx - cfg->coord.leftx);
	/* set mapcolor */
	osd_colormap_ctrl(cfg->nr, cfg->mapcolor.enable, cfg->mapcolor.color);
	/* set dma buf addr */
	osd_set_dma_channel(cfg->nr, cfg->dma.dma_num, cfg->dma.buf);
	/* set swap */
	osd_set_pixel_swap(cfg->nr, &cfg->swap);
	/* set pallete */
	osd_set_pallete(cfg->nr, &cfg->pallete);
	/* if win1, we need to set alpha and colorkey */
	if (cfg->nr) {
		osd_set_alpha(cfg->nr, &cfg->alpha);
		osd_set_colorkey(cfg->nr, &cfg->colorkey);
	}
}

void osd_open(int nr)
{
	osd_enable_ctrl(nr, 1);
}

void osd_close(int nr)
{
	osd_enable_ctrl(nr, 0);
}

void osd_init(struct osd_cfg *cfg)
{
	uint32_t val;
        /*
        *(1)参数配置了隔行逐行扫描模式
        *(2)参数配置了itu接口模式（输出）
        *(3)参数配置了全局变量使能
        *(4)参数配置了DMA错误超时时间
        */
	val = (cfg->ituInterface << OVCDCR_Interlace) | 
		  (cfg->interlaced << OVCDCR_IfType) |
		  (1 << OVCDCR_LOAD_EN) | (0xf << OVCDCR_WaitTime);
	if (cfg->mode < BOTH_CHANNEL)
		val |= ((cfg->mode + 1) << OVCDCR_SP_LOAD_EN) << 0;
	writel(val, reg_base + OVCDCR);
	/*
	*(1)写0.往调色板控制寄存器的bit15写0，表示让调色板处于normal模式。
	*(2)In normal mode, cannot update palette memory
	*/
	osd_palette_ctrl(DISABLE);
	/*
	*(1)set background color blue
	*(2)设置osd背景颜色为0，那就是白色。
	*/
	osd_set_bg_color(cfg->gbcolor);
	/* 
	*(1)set scale xy 
	*(2)通过配置osd的窗口大小配置寄存器，来配置osd图层的宽度和高度。当且仅当有scaler模块时有效。
	*/
	osd_set_scale_xy(cfg->screenWidth, cfg->screenHeight);
}

void osd_deinit(void)
{

}
