#include <i80.h>
#include <ids/ids.h>
#include <irq.h>

static struct reg_check reg[] = {
	REG_CHECK_INIT(I80IFCON0,       	0x0ffffbff),
	REG_CHECK_INIT(I80IFCON1,		0x0000ff1f),
	REG_CHECK_INIT(I80CMDCON0,		0xffffffff),
	REG_CHECK_INIT(I80CMDCON1,		0x55555555),
	REG_CHECK_INIT(I80CMD15,		0x0003ffff),
	REG_CHECK_INIT(I80CMD14,		0x0003ffff),
	REG_CHECK_INIT(I80CMD13,		0x0003ffff),
	REG_CHECK_INIT(I80CMD12,		0x0003ffff),
	REG_CHECK_INIT(I80CMD11,		0x0003ffff),
	REG_CHECK_INIT(I80CMD10,		0x0003ffff),
	REG_CHECK_INIT(I80CMD09,		0x0003ffff),
	REG_CHECK_INIT(I80CMD08,		0x0003ffff),
	REG_CHECK_INIT(I80CMD07,		0x0003ffff),
	REG_CHECK_INIT(I80CMD06,		0x0003ffff),
	REG_CHECK_INIT(I80CMD05,		0x0003ffff),
	REG_CHECK_INIT(I80CMD04,		0x0003ffff),
	REG_CHECK_INIT(I80CMD03,		0x0003ffff),
	REG_CHECK_INIT(I80CMD02,		0x0003ffff),
	REG_CHECK_INIT(I80CMD01,		0x0003ffff),
	REG_CHECK_INIT(I80CMD00,		0x0003ffff),
	REG_CHECK_INIT(I80MANCON,		0x0000007f),
	REG_CHECK_INIT(I80MANWDAT,		0x0003ffff),
};

int i80_reg_check(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(reg); i++)
		reg[i].addr += reg_base;

	if (common_reg_check(reg, ARRAY_SIZE(reg), 0xffffffff) || common_reg_check(reg, ARRAY_SIZE(reg), 0))
		return -1;

	return 0;
}

static void i80_writel(uint32_t offset, uint32_t data,
					uint8_t shift, uint8_t width)
{
	uint32_t mask = (1 << width) - 1;
	uint32_t val;

	val = readl(reg_base + offset);
	val &= ~(mask << shift);
	val |= (data & mask) << shift;
	writel(val, reg_base + offset);
}

static uint32_t i80_readl(uint32_t offset, uint8_t shift, uint8_t width)
{
	uint32_t mask = (1 << width) - 1;
	uint32_t val;

	val = readl(reg_base + offset);
	val &= (mask << shift);
	val >>= shift;

	return val;
}

int i80_wait_idle(void)
{
	uint32_t val;

	while (1) {		
		val = i80_readl(I80TRIGCON, 0, 5);
		if (!(val &= ((1 << I80IF_TRIGCON_NORMAL_CMD_ST) |
			(1 << I80IF_TRIGCON_TR_DATA) |
			(1 << I80IF_TRIGCON_TR_AUTOCMD) |
			(1 << I80IF_TRIGCON_TR_NORMALCMD))))
			break;
	}
	return 0;
}

void i80_wait_frame_end(void)
{
	uint32_t val;
	
	do {	
		val = i80_readl(I80TRIGCON, 4, 1);
	} while (!val);
}

void i80_enable(int flags)
{
	i80_writel(I80IFCON0, flags, I80IF_IFCON0_EN, 1);

}

void i80_set_clock_cycle(struct i80_cycles *cycle)
{
	if (!cycle)
		return;

	i80_writel(I80IFCON0, cycle->cs_setup, I80IF_IFCON0_CS_SETUP, 5);
	i80_writel(I80IFCON0, cycle->wr_setup, I80IF_IFCON0_WR_SETUP, 6);
	i80_writel(I80IFCON0, cycle->wr_act, I80IF_IFCON0_WR_ACTIVE, 6);
	i80_writel(I80IFCON0, cycle->wr_hold, I80IF_IFCON0_WR_HOLD, 6);
}

void i80_set_int_mask(enum I80_INT_TYPE type)
{
	i80_writel(I80IFCON0, type, I80IF_IFCON0_INTR_EN, 2); 
}

void i80_set_main_lcd(enum MAIN_LCD type)
{
	i80_writel(I80IFCON0, type, I80IF_IFCON0_MAINLCD_CFG, 1);
}

void i80_set_date_structure(struct i80_data *data)
{
	if (!data)
		return;

	i80_writel(I80IFCON1, data->porttype, I80IF_IFCON1_PORTTYPE, 2);
	i80_writel(I80IFCON1, data->portdistributed, I80IF_IFCON1_PORTDISTRIBUTED, 2);
	i80_writel(I80IFCON1, data->datastyle, I80IF_IFCON1_DATASTYLE, 2);
	i80_writel(I80IFCON1, data->datalsb, I80IF_IFCON1_DATALSBFIRST, 1);
	i80_writel(I80IFCON1, data->datauselowport, I80IF_IFCON1_DATAUSELOWPORT, 1);
}

void i80_set_autocmd(struct i80_autocmd *cmd)
{
	if (!cmd)
		return;

	i80_writel(I80IFCON1, cmd->type, I80IF_IFCON1_DISAUTOCMD, 1);
	i80_writel(I80IFCON1, cmd->rate, I80IF_IFCON1_AUTOCMDRATE, 4);
}

void i80_set_cmd(struct i80_cmd *cmd) 
{
	if (!cmd)
		return;

	i80_writel(I80CMDCON0, cmd->type, cmd->num * 2, 2);
	i80_writel(I80CMDCON1, cmd->rs_ctrl, cmd->num * 2, 1);
	i80_writel(I80CMD15 + (15 - cmd->num) * 4, cmd->cmd, 0, 18);
}

void i80_mannual_write_once(int data)
{
	i80_writel(I80MANWDAT, data, 0, 18);
	i80_mannual_ctrl(I80_MAN_DOE, I80_ACTIVE_HIGH);
	udelay(1);
	i80_mannual_ctrl(I80_MAN_WR, I80_ACTIVE_LOW);
	udelay(10);
	i80_mannual_ctrl(I80_MAN_WR, I80_ACTIVE_HIGH);
	udelay(10);
}

int i80_mannual_read_once(void)
{
	uint32_t val;

	i80_mannual_ctrl(I80_MAN_DOE, I80_ACTIVE_LOW);
	udelay(1);
	i80_mannual_ctrl(I80_MAN_RD, I80_ACTIVE_LOW);
	udelay(15);
	val = i80_readl(I80MANRDAT, 0, 18);	
	i80_mannual_ctrl(I80_MAN_RD, I80_ACTIVE_HIGH);
	return val;
}

void i80_mannual_ctrl(enum SIGNAL_CTRL ctrl, int value)
{
	i80_writel(I80MANCON, value, ctrl, 1);	
}

void i80_mannual_init(void)
{
	i80_writel(I80MANCON, 0x3f, 0, 6);
}

void i80_mannual_deinit(void)
{
	i80_writel(I80MANCON, 0x00, 0, 6);
}

void i80_trigger_normal_cmd(int flags)
{
	i80_writel(I80TRIGCON, flags, 0, 1);
}

void i80_clear_cmd_list(void)
{
	struct i80_cmd cmd;
	int i;
	
	for (i = 0; i < 16; i++) {
		cmd.num = i;
		cmd.cmd = 0;
		cmd.type = CMD_DISABLE;
		cmd.rs_ctrl = I80_ACTIVE_LOW;
		i80_set_cmd(&cmd);
	}
}

static int data_type[5][5] = {
	{ 0x0, 0x0, 0x0, 0x0, 0x0 },
	{ 0x1, 0x0, 0x0, 0x0, 0x0 },
	{ 0x2, 0x1, 0x0, 0x0, 0x0 },
	{ 0x3, 0x0, 0x0, 0x0, 0x0 },
	{ 0x2, 0x3, 0x0, 0x0, 0x0 },
};

void i80_default_cfg(int format)
{
	struct i80_data data;
	struct i80_autocmd autocmd;
	struct i80_cycles cycles;
	
	i80_wait_idle();

	memcpy(&data, data_type[format], 5 * 4);
	i80_set_date_structure(&data);
		
	autocmd.type = AUTOCMD_BEFORE_VIDEO;	
	autocmd.rate = AUTO_RATE_PER_1;
	i80_set_autocmd(&autocmd);
	
	i80_clear_cmd_list();
	i80_mannual_deinit();		
	
	cycles.cs_setup = 0;
	cycles.wr_setup = 0;
	cycles.wr_act = 1;
	cycles.wr_hold = 0;
	i80_set_clock_cycle(&cycles);	

	i80_set_int_mask(FRAME_OR_CMD_OVER);
	i80_set_main_lcd(CS0_MAIN_LCD);	
}

int i80_frame_irq(void)
{
	uint32_t status;
	int ret = -1;

	status = readl(reg_base + IDSINTPND);
	if (status & (1 << IDSINTPND_I80INT)) {
		ids_dbg("success to send a frame\n");
		ret = 0;
		irq_num++;
	}
	writel(status, reg_base + IDSSRCPND);
	writel(status, reg_base + IDSINTPND);
	
	return ret;
}

int i80_irq_mode(void)
{
	int ret;
	printf("i80_irq_mode\n");
	ret = request_irq(IDS1_INT_ID, (irq_handler_t)i80_frame_irq, "i80");	
	if (ret < 0) {
		ids_err("i80 request irq err\n");	
		return -1;
	}
	return 0;
}

int i80_init(int num)
{
	lcd_set_output_if(I80_LCD);
	if (num) {
	//	module_set_clock("ids1-eitf", "epll", 54000000, 0);
		;
	} else {
		//ids_err("Q3 do not support ids0\n");
		//return -1;
		;
	}
	return 0;
}
