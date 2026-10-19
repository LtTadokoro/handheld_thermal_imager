#include <stdio.h>
#include "menu.h"
#include "UI.h"
#include "MLX90640_API.h"
#include "gpio.h"
#include "image_processing.h"
#include "buffer.h"
#include "task_management.h"
#include "nvs_wr.h"

#define SUB_PAGE_TRUE 0
#define SUB_PAGE_FALSE 1
#define TOGGLE_TRUE 1
#define TOGGLE_FALSE 0
#define STATE_ON 1
#define STATE_OFF 0

// 菜单项状态
typedef struct MenuNode 
{
    char* text;                // 显示文本
    //uint8_t text_len;          // 文本长度（用于快速计算宽度，节省 GetTextWidth）
    uint8_t idx;               // 每个节点独一无二的索引编号
    struct MenuNode* parent;   // 父节点
    struct MenuNode* child;    // 第一个子节点
    struct MenuNode* prev;     // 前一个兄弟
    struct MenuNode* next;     // 后一个兄弟
    uint8_t is_leaf;           // 0: 有子菜单, 1: 无子菜单（叶子节点）
    uint8_t is_toggle;         // 1: 切换项（如 on/off），0: 普通进入项
    char* toggle_text_on;      // 切换状态为 on 时的文本（如果 is_toggle == 1）
    char* toggle_text_off;     // 切换状态为 off 时的文本
    uint8_t toggle_state;      // 当前切换状态 0=off, 1=on
    void (*action)(struct MenuNode* self);   // 执行函数，参数为自身节点
} MenuNode;

// 全局菜单状态
MenuNode* g_current_node = NULL;        // 当前高亮显示的节点（当前选中的菜单项）
MenuNode* g_current_parent = NULL;      // 当前显示的菜单的父节点（即当前菜单层级）
uint8_t g_selected_index = 0;           // 当前选中项在兄弟列表中的序号（用于上下移动）
static int8_t g_scroll_dir_override = 0;   // 0=自动判断, 非0=强制方向

// 预声明所有节点
static MenuNode node_settings, node_function;
static MenuNode node_set_refresh_rate, node_interpolation_method, node_buzzer_on_off, node_factory_reset;
static MenuNode node_color_scheme, node_hot_cold_points, node_laser_pointer_on_off, node_units_of_temp;
static MenuNode node_2Hz, node_4Hz, node_8Hz, node_16Hz, node_32Hz;
static MenuNode node_neighbor, node_double_linear;
static MenuNode node_yes, node_no;
static MenuNode node_color_1, node_color_2, node_color_3, node_color_4, node_color_5, node_color_6;
static MenuNode node_hot_point_on_off, node_cold_point_on_off;
static MenuNode node_celsius, node_fahrenheit, node_kelvin;

static void action_toggle_buzzer(MenuNode* self);
static void action_toggle_crossing(MenuNode* self);
static void action_toggle_laser(MenuNode* self);
static void action_set_refresh_rate(MenuNode* self);
static void action_set_color_scheme(MenuNode* self);
static void action_set_interpolation_method(MenuNode* self);
static void action_set_factory_reset(MenuNode* self);
static void switch_temp_units(MenuNode* self);

// 初始化菜单树
void Menu_Init(void) 
{
    // ==================== 三级叶子节点（频率） ====================
    node_2Hz = (MenuNode){"--2Hz", 1, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, action_set_refresh_rate};
    node_4Hz = (MenuNode){"--4Hz", 2, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, action_set_refresh_rate};
    node_8Hz = (MenuNode){"--8Hz", 3, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, action_set_refresh_rate};
    node_16Hz = (MenuNode){"--16Hz", 4, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, action_set_refresh_rate};
    node_32Hz = (MenuNode){"--32Hz", 5, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, action_set_refresh_rate};
    // 链接兄弟
    node_2Hz.prev = &node_32Hz;     node_2Hz.next = &node_4Hz;      
    node_4Hz.prev = &node_2Hz;      node_4Hz.next = &node_8Hz;      
    node_8Hz.prev = &node_4Hz;      node_8Hz.next = &node_16Hz;     
    node_16Hz.prev = &node_8Hz;     node_16Hz.next = &node_32Hz;    
    node_32Hz.prev = &node_16Hz;    node_32Hz.next = &node_2Hz;

    // ==================== 三级叶子节点（插值） ====================
    node_neighbor = (MenuNode){"--neighbor", 6, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, action_set_interpolation_method};
    node_double_linear = (MenuNode){"--bilinear", 7, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, action_set_interpolation_method};
    node_neighbor.prev = &node_double_linear;
    node_neighbor.next = &node_double_linear;
    node_double_linear.prev = &node_neighbor;
    node_double_linear.next = &node_neighbor;

    // ==================== 三级叶子节点（factory reset） ====================
    node_yes = (MenuNode){"--yes", 8, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, action_set_factory_reset};
    node_no = (MenuNode){"--no", 9, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, action_set_factory_reset};
    node_yes.prev = &node_no;   node_yes.next = &node_no;
    node_no.prev = &node_yes;   node_no.next = &node_yes;

    // ==================== 三级叶子节点（color scheme） ====================
    node_color_1 = (MenuNode){"--gray", 10, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, action_set_color_scheme};
    node_color_2 = (MenuNode){"--metal", 11, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, action_set_color_scheme};
    node_color_3 = (MenuNode){"--pseudo 1", 12, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, action_set_color_scheme};
    node_color_4 = (MenuNode){"--pseudo 2", 13, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, action_set_color_scheme};
    node_color_5 = (MenuNode){"--rainbow 1", 14, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, action_set_color_scheme};
    node_color_6 = (MenuNode){"--rainbow 2", 15, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, action_set_color_scheme};

    // 链接（注意顺序）
    node_color_1.prev = &node_color_6;    node_color_1.next = &node_color_2;
    node_color_2.prev = &node_color_1;    node_color_2.next = &node_color_3;
    node_color_3.prev = &node_color_2;    node_color_3.next = &node_color_4;
    node_color_4.prev = &node_color_3;    node_color_4.next = &node_color_5;
    node_color_5.prev = &node_color_4;    node_color_5.next = &node_color_6;
    node_color_6.prev = &node_color_5;    node_color_6.next = &node_color_1;

    // ==================== 三级叶子节点（units of temp） ====================
    node_celsius = (MenuNode){"--Celsius", 29, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, switch_temp_units};
    node_fahrenheit = (MenuNode){"--Fahrenheit", 30, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, switch_temp_units};
    node_kelvin = (MenuNode){"--Kelvin", 31, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, switch_temp_units};

    // 链接
    node_celsius.prev = &node_kelvin;
    node_celsius.next = &node_fahrenheit;
    node_fahrenheit.prev = &node_celsius;
    node_fahrenheit.next = &node_kelvin;
    node_kelvin.prev = &node_fahrenheit;
    node_kelvin.next = &node_celsius;

    // ==================== 三级叶子节点（hot/cold points 子项） ====================
    // 冷热点绘制默认开启，默认显示off，切换后变成on
    node_hot_point_on_off = (MenuNode){"--hot off", 16, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_TRUE, "--hot on", "--hot off", !hot_crossing, action_toggle_crossing};
    node_cold_point_on_off = (MenuNode){"--cold off", 17, NULL, NULL, NULL, NULL, SUB_PAGE_FALSE, TOGGLE_TRUE, "--cold on", "--cold off", !cold_crossing, action_toggle_crossing};
    node_hot_point_on_off.prev = &node_cold_point_on_off;
    node_hot_point_on_off.next = &node_cold_point_on_off;
    node_cold_point_on_off.prev = &node_hot_point_on_off;
    node_cold_point_on_off.next = &node_hot_point_on_off;

    // ==================== 二级节点 ====================
    // set refresh rate
    node_set_refresh_rate = (MenuNode){"-refresh rate", 18, NULL, &node_2Hz, NULL, &node_interpolation_method, SUB_PAGE_TRUE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, NULL};
    node_2Hz.parent = &node_set_refresh_rate;
    node_4Hz.parent = &node_set_refresh_rate;
    node_8Hz.parent = &node_set_refresh_rate;
    node_16Hz.parent = &node_set_refresh_rate;
    node_32Hz.parent = &node_set_refresh_rate;

    // interpolation method
    node_interpolation_method = (MenuNode){"-interpolation", 19, NULL, &node_neighbor, &node_set_refresh_rate, &node_buzzer_on_off, SUB_PAGE_TRUE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, NULL};
    node_neighbor.parent = &node_interpolation_method;
    node_double_linear.parent = &node_interpolation_method;

    // buzzer on/off (toggle)
    node_buzzer_on_off = (MenuNode){"-buzzer off", 20, NULL, NULL, &node_interpolation_method, &node_factory_reset, SUB_PAGE_FALSE, TOGGLE_TRUE, "-buzzer on", "-buzzer off", !buzzer_level, action_toggle_buzzer};

    // factory reset
    node_factory_reset = (MenuNode){"-factory reset", 21, NULL, &node_yes, &node_buzzer_on_off, NULL, SUB_PAGE_TRUE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, NULL};
    node_yes.parent = &node_factory_reset;
    node_no.parent = &node_factory_reset;

    // color scheme
    node_color_scheme = (MenuNode){"-color scheme", 22, NULL, &node_color_1, NULL, &node_hot_cold_points, SUB_PAGE_TRUE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, NULL};
    node_color_1.parent = &node_color_scheme;
    node_color_2.parent = &node_color_scheme;
    node_color_3.parent = &node_color_scheme;
    node_color_4.parent = &node_color_scheme;
    node_color_5.parent = &node_color_scheme;
    node_color_6.parent = &node_color_scheme;

    // hot cold points
    node_hot_cold_points = (MenuNode){"-hot/cold spot", 23, NULL, &node_hot_point_on_off, &node_color_scheme, &node_laser_pointer_on_off, SUB_PAGE_TRUE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, NULL};
    node_hot_point_on_off.parent = &node_hot_cold_points;
    node_cold_point_on_off.parent = &node_hot_cold_points;

    // laser pointer on/off (toggle)
    node_laser_pointer_on_off = (MenuNode){"-laser on", 27, NULL, NULL, &node_hot_cold_points, &node_units_of_temp, SUB_PAGE_FALSE, TOGGLE_TRUE, "-laser on", "-laser off", !laser_level, action_toggle_laser};

    // units of temp
    node_units_of_temp = (MenuNode){"-units of temp", 28, NULL, &node_celsius, &node_laser_pointer_on_off, NULL, SUB_PAGE_TRUE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, NULL};
    node_celsius.parent = &node_units_of_temp;
    node_fahrenheit.parent = &node_units_of_temp;
    node_kelvin.parent = &node_units_of_temp;

    // ==================== 二级节点环形链接（settings 的子项） ====================
    node_set_refresh_rate.prev = &node_factory_reset;
    node_set_refresh_rate.next = &node_units_of_temp;
    node_units_of_temp.prev = &node_set_refresh_rate;
    node_units_of_temp.next = &node_interpolation_method;
    node_interpolation_method.prev = &node_units_of_temp;
    node_interpolation_method.next = &node_buzzer_on_off;
    node_buzzer_on_off.prev = &node_interpolation_method;
    node_buzzer_on_off.next = &node_factory_reset;
    node_factory_reset.prev = &node_buzzer_on_off;
    node_factory_reset.next = &node_set_refresh_rate;

    // ==================== 二级节点环形链接（function 的子项） ====================
    node_color_scheme.prev = &node_laser_pointer_on_off;
    node_color_scheme.next = &node_hot_cold_points;
    node_hot_cold_points.prev = &node_color_scheme;
    node_hot_cold_points.next = &node_laser_pointer_on_off;
    node_laser_pointer_on_off.prev = &node_hot_cold_points;
    node_laser_pointer_on_off.next = &node_color_scheme;
    
    // ==================== 一级节点 ====================

    node_settings = (MenuNode){"settings", 25, NULL, &node_set_refresh_rate, NULL, &node_function, SUB_PAGE_TRUE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, NULL};
    node_function = (MenuNode){"function", 26, NULL, &node_color_scheme, &node_settings, NULL, SUB_PAGE_TRUE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, NULL};
    node_set_refresh_rate.parent = &node_settings;
    node_interpolation_method.parent = &node_settings;
    node_buzzer_on_off.parent = &node_settings;
    node_factory_reset.parent = &node_settings;

    // 设置二级节点的父节点
    node_set_refresh_rate.parent = &node_settings;
    node_interpolation_method.parent = &node_settings;
    node_buzzer_on_off.parent = &node_settings;
    node_factory_reset.parent = &node_settings;

    node_color_scheme.parent = &node_function;
    node_hot_cold_points.parent = &node_function;
    node_laser_pointer_on_off.parent = &node_function;

    // ==================== 一级节点环形链接 ====================
    node_settings.prev = &node_function;
    node_settings.next = &node_function;
    node_function.prev = &node_settings;
    node_function.next = &node_settings;

    // 设置一级节点的父节点为根（虚拟根）
    static MenuNode root;
    root = (MenuNode){"", 0, NULL, &node_function, NULL, NULL, SUB_PAGE_TRUE, TOGGLE_FALSE, NULL, NULL, STATE_OFF, NULL};
    node_settings.parent = &root;
    node_function.parent = &root;

    // 初始化全局状态：默认选中第一个一级菜单（settings）
    g_current_node = &node_function;
    g_current_parent = &root;
    g_selected_index = 0;

    menu_display_current();
}

// 进入子菜单
void menu_enter_child(void) 
{
    if (g_current_node == NULL) return;
    if (g_current_node->child != NULL) 
    {
        // 进入子菜单，保存父节点
        g_current_parent = g_current_node;
        g_current_node = g_current_node->child;
        g_selected_index = 0;

        g_scroll_dir_override = +1;   // 进入子菜单强制“向下滚动”（如不够，改成 -1）
        menu_display_current();       // 触发显示更新（滚动动画切换到当前节点）
        g_scroll_dir_override = 0;    // 立即复位，避免影响其他调用

        return;
    } 
    else 
    {
        // 叶子节点：执行回调
        if (g_current_node->action != NULL) 
        {
            g_current_node->action(g_current_node);
        }
    }
}

// 返回父菜单
void menu_goto_parent(void) 
{
    if (g_current_parent != NULL) 
    {
        g_current_node = g_current_parent;
        g_current_parent = g_current_parent->parent;
        g_selected_index = 0; // 或恢复之前的选中索引（可存储）
        menu_display_current();
    }
}

// 上下移动
void menu_move_up(void) 
{
    if (g_current_node == NULL) return;
    if (g_current_node->prev != NULL) 
    {
        g_current_node = g_current_node->prev;
        g_selected_index--;
        menu_display_current();  // 滚动向上切换
    }
}

void menu_move_down(void) 
{
    if (g_current_node == NULL) return;
    if (g_current_node->next != NULL) 
    {
        g_current_node = g_current_node->next;
        g_selected_index++;
        menu_display_current();  // 滚动向下切换
    }
}

// 显示当前菜单项（整合滚动动画）
char* get_display_text(MenuNode* node) 
{
    if (node->is_toggle) 
    {
        return node->toggle_state ? node->toggle_text_on : node->toggle_text_off;
    } 
    else 
    {
        return node->text;
    }
}

void menu_display_current(void) 
{
    if (g_current_node == NULL) return;
    char* text = get_display_text(g_current_node);
    // 如果当前显示区域没有文本，或者文本与上次不同，触发滚动
    static char* last_text = NULL;
    //static int last_direction = 0; // 1=down, -1=up
    if (last_text != text) 
    {
        static uint8_t last_index = 0;
        int direction;
        if (g_scroll_dir_override != 0) 
        {
            direction = g_scroll_dir_override;
        }
        else
        {
            direction = (g_selected_index > last_index) ? 1 : -1;
        }
        MenuScrollTo(text, direction);
        last_index = g_selected_index;
        last_text = text;
    } 
    else 
    {
        // 同一文本（可能是切换状态变化），直接刷新显示，不滚动
        MenuScrollTo(text, 0); // 0表示不滚动，立即显示
    }
}

// 传感器刷新率选择回调
void action_set_refresh_rate(MenuNode* self) 
{
    write_data(REFRESHRATE_KEY_INDEX, self->idx);
    switch (self->idx)
    {
        case RefreshRate_2Hz_idx:
            Set_RefreshRate_2Hz();
            break;
        case RefreshRate_4Hz_idx:
            Set_RefreshRate_4Hz();
            break;
        case RefreshRate_8Hz_idx:
            Set_RefreshRate_8Hz();
            break;
        case RefreshRate_16Hz_idx:
            Set_RefreshRate_16Hz();
            break;
        case RefreshRate_32Hz_idx:
            Set_RefreshRate_32Hz();
            break;
    }
    //menu_display_current();
}

// 色彩方案选择回调
void action_set_color_scheme(MenuNode* self) 
{
    write_data(SCHEME_KEY_INDEX, self->idx);
    switch (self->idx)
    {
        case SCHEME_GRAY_INDEX:
            Set_Color_Scheme(COLOR_SCHEME_GRAY);
            break;
        case SCHEME_METAL_INDEX:
            Set_Color_Scheme(COLOR_SCHEME_METAL);
            break;
        case SCHEME_PSEUDO1_INDEX:
            Set_Color_Scheme(COLOR_SCHEME_PSEUDO1);
            break;
        case SCHEME_PSEUDO2_INDEX:
            Set_Color_Scheme(COLOR_SCHEME_PSEUDO2);
            break;
        case SCHEME_RAINBOW1_INDEX:
            Set_Color_Scheme(COLOR_SCHEME_RAINBOW1);
            break;
        case SCHEME_RAINBOW2_INDEX:
            Set_Color_Scheme(COLOR_SCHEME_RAINBOW2);
            break;
    }
    //menu_display_current();
}

// 插值算法选择回调
void action_set_interpolation_method(MenuNode* self)
{
    switch (self->idx)
    {
        case 6:
            interpolation_method = 0;
            break;
        case 7:
            interpolation_method = 1;
            break;
    }
    write_data(INTERPOLATION_KEY_INDEX, interpolation_method);
    //menu_goto_parent();
    //menu_display_current();
}

// 恢复出厂设置选择回调
void action_set_factory_reset(MenuNode* self)
{
    switch (self->idx)
    {
        case 8:
            Factory_Reset();
            break;
        case 9:

            break;
    }
    //menu_display_current();
    menu_goto_parent();
}

// 切换蜂鸣器开关回调
void action_toggle_buzzer(MenuNode* self) 
{
    buzzer_level = !buzzer_level;
    write_data(BUZZER_KEY_INDEX, buzzer_level);
    self->toggle_state ^= 1;
    menu_display_current();
}

// 切换激光管开关回调
void action_toggle_laser(MenuNode* self) 
{
    // laser节点的初始状态是on（1），激光管实际初始状态是关闭（0），因此直接打开，再改变状态
    turn_laser(self->toggle_state);

    self->toggle_state ^= 1;
    menu_display_current();
}

// 切换冷热点是否显示 回调
void action_toggle_crossing(MenuNode* self) 
{
    // 冷热点节点文本初始状态是off（0），冷热点实际状态是打开（1），因此触发时先传值再改变状态
    if (self->idx == 16)
    {
        hot_crossing = !hot_crossing;
        write_data(HOT_KEY_INDEX, hot_crossing);
    }
    else
    {
        cold_crossing = !cold_crossing;
        write_data(COLD_KEY_INDEX, hot_crossing);
    }
    self->toggle_state ^= 1;
    menu_display_current();
}

// 切换温度单位回调
void switch_temp_units(MenuNode* self)
{
    switch(self->idx)
    {
        case 29:
            temp_units_type = CELSIUS;
            break;
        case 30:
            temp_units_type = FAHRENHEIT;
            break;
        case 31:
            temp_units_type = KELVIN;
            break;
    }
    write_data(TEMP_UNITS_KEY_INDEX, temp_units_type);
}