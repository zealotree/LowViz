#include <pebble.h>
Window *my_window;
static Layer *main_clock;
static Layer *clock_hands;
static GPath *hour_arrow;
static GTextAttributes *s_attributes;
static char date_buffer[] = "31";


static const GPathInfo HOUR_HAND_POINTS = {
  9, (GPoint []){
    { -2, 0 },
	{ 2, 0 },
	{ 2, -20 },
	{ 4, -20 },
    { 4, -59 },
	{ 0, -63 },
	{ -4, -59 },
	{ -4, -20 },
	{ -2, -20 }
  }
};

// Layout boundaries
#define TIME_RING PBL_IF_ROUND_ELSE(23, 20)
#define DOT_RING PBL_IF_ROUND_ELSE(52, 20)
#define DATE_RING PBL_IF_ROUND_ELSE(75, 20)


// Bitmaps for Hours
// H0 is 12 and 00
GBitmap *H0; GBitmap *H1; GBitmap *H2; GBitmap *H3; GBitmap *H4;
GBitmap *H5; GBitmap *H6; GBitmap *H7; GBitmap *H8; GBitmap *H9;
GBitmap *H10; GBitmap *H11;


static int32_t get_angle_for_hour(int hour) {
  // Progress through 12 hours, out of 360 degrees
  return (hour * 360) / 12;
}


GBitmap* getBitmapFromHour(int num){
	switch(num){
	case 0:
		return H0;
	case 1:
		return H1;
	case 2:
		return H2;
	case 3:
		return H3;
	case 4:
		return H4;
	case 5:
		return H5;
	case 6:
		return H6;
	case 7:
		return H7;
	case 8:
		return H8;
	case 9:
		return H9;
	case 10:
		return H10;
	case 11:
		return H11;
	case 12:
		return H0;
	default:
		return NULL;
	}
}

static int32_t get_angle_for_minute(int hour) {
  // Progress through 60 miunutes, out of 360 degrees
  return (hour * 360) / 60;
}

static void draw_main_clock(Layer *layer, GContext *ctx) {
  
  GRect bounds = layer_get_bounds(layer);
  GRect time_ring = grect_inset(bounds, GEdgeInsets(TIME_RING));
  GRect dot_ring = grect_inset(bounds, GEdgeInsets(DOT_RING));
  
  	GRect frame = grect_inset(bounds, GEdgeInsets(2));
	GRect inner_hour_frame = grect_inset(bounds, GEdgeInsets((4 * 1) + 2));
GRect inner_minute_frame = grect_inset(bounds, GEdgeInsets((4 * 1) + 2));
  
	// Minute Marks
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_stroke_width(ctx, 1);
	for(int i = 0; i < 60; i++) {
		if (i % 5) {
			int minute_angle = get_angle_for_minute(i);
			GPoint p0 = gpoint_from_polar(frame, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(minute_angle));
			GPoint p1 = gpoint_from_polar(inner_minute_frame, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(minute_angle));
			graphics_draw_line(ctx, p0, p1);
		}
	}


  
  // Draw Hour Markers
  for(int i = 0; i < 13; i++) {
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_context_set_fill_color(ctx, GColorBlack);
    GRect digits = grect_centered_from_polar(time_ring, GOvalScaleModeFitCircle, 
                                             DEG_TO_TRIGANGLE(get_angle_for_hour(i)), GSize(33,33));
    graphics_draw_bitmap_in_rect(ctx, getBitmapFromHour(i), digits);
    
        graphics_context_set_fill_color(ctx, GColorBlack);
    GPoint tick = gpoint_from_polar(dot_ring, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(get_angle_for_hour(i)));
    graphics_fill_circle(ctx, tick, 2);
    
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_stroke_width(ctx, 1);
    int hour_angle = get_angle_for_hour(i);
    GPoint p0 = gpoint_from_polar(frame, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(hour_angle));
		GPoint p1 = gpoint_from_polar(inner_hour_frame, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(hour_angle));
		graphics_draw_line(ctx, p0, p1);
  }
  


}

static void draw_clock_hands(Layer *layer, GContext *ctx) {
  
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  
    GRect date_ring = grect_inset(bounds, GEdgeInsets(DATE_RING));

  
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  


  // Draw Minute Hand
  int minute_angle = TRIG_MAX_ANGLE * tick_time->tm_min / 60;
  
  int minute_hand_length = bounds.size.w / 2 - 5;

  GPoint minute_hand = {
    .x = (int16_t)(sin_lookup(minute_angle) * (int32_t)  minute_hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(minute_angle) * (int32_t) minute_hand_length / TRIG_MAX_RATIO) + center.y,
  };
  
  graphics_context_set_stroke_width(ctx, 4);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_line(ctx, center, minute_hand);

  
	graphics_context_set_fill_color(ctx, GColorBlack);
	gpath_rotate_to(hour_arrow, (TRIG_MAX_ANGLE * (((tick_time->tm_hour % 12) * 6) + (tick_time->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, hour_arrow);
  
    // Draw Second
  
  if (tick_time->tm_hour >= 23 && tick_time->tm_hour <= 5) {
    // Do not draw seconds from 23:00 to 5:00
  } else {
    int second_angle = TRIG_MAX_ANGLE * tick_time->tm_sec / 60;
    int32_t second_angle_tail = TRIG_MAX_ANGLE * (tick_time->tm_sec + 30) / 60;

    int16_t second_hand_length = bounds.size.w / 2 - 5;
    int16_t second_hand_tail_length = 18;

    GPoint second_hand = {
      .x = (int16_t)(sin_lookup(second_angle) * (int32_t) second_hand_length / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(second_angle) * (int32_t) second_hand_length / TRIG_MAX_RATIO) + center.y,
    };

    GPoint second_hand_tail = {
      .x = (int16_t)(sin_lookup(second_angle_tail) * (int32_t)second_hand_tail_length / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(second_angle_tail) * (int32_t)second_hand_tail_length / TRIG_MAX_RATIO) + center.y,
    };


    graphics_context_set_stroke_width(ctx, 2  );
    graphics_context_set_stroke_color(ctx, GColorRed);
    graphics_draw_line(ctx, center, second_hand);
    graphics_context_set_stroke_width(ctx, 3);
    graphics_draw_line(ctx, second_hand_tail, center);
  }

  

  // Draw Center Dot
  graphics_fill_circle(ctx, center, 5);
  
  // Draw Current Date
  strftime(date_buffer, sizeof(date_buffer), "%d", tick_time);  
    GRect date = grect_centered_from_polar(date_ring, GOvalScaleModeFitCircle, 
                                             DEG_TO_TRIGANGLE(get_angle_for_hour(6)), GSize(25,25));
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, date_buffer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), date,
                GTextOverflowModeWordWrap, GTextAlignmentCenter, s_attributes);
  
  
}

static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  if (SECOND_UNIT & changed) {
      layer_mark_dirty(clock_hands);
  }
}

static void main_window_load() {
  window_set_background_color(my_window, GColorWhite);
  


  Layer *window_layer = window_get_root_layer(my_window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);

  main_clock = layer_create(bounds);
  clock_hands = layer_create(bounds);
  
  hour_arrow = gpath_create(&HOUR_HAND_POINTS);
  gpath_move_to(hour_arrow, center);
  
	H0 = gbitmap_create_with_resource(RESOURCE_ID_RES_12); H1 = gbitmap_create_with_resource(RESOURCE_ID_RES_1);
	H2 = gbitmap_create_with_resource(RESOURCE_ID_RES_2); H3 = gbitmap_create_with_resource(RESOURCE_ID_RES_3);
	H4 = gbitmap_create_with_resource(RESOURCE_ID_RES_4); H5 = gbitmap_create_with_resource(RESOURCE_ID_RES_5);
	H6 = gbitmap_create_with_resource(RESOURCE_ID_RES_6); H7 = gbitmap_create_with_resource(RESOURCE_ID_RES_7);
	H8 = gbitmap_create_with_resource(RESOURCE_ID_RES_8); H9 = gbitmap_create_with_resource(RESOURCE_ID_RES_9);
  H10 = gbitmap_create_with_resource(RESOURCE_ID_RES_10); H11 = gbitmap_create_with_resource(RESOURCE_ID_RES_11);

  
  layer_set_update_proc(main_clock, draw_main_clock);  
  layer_set_update_proc(clock_hands, draw_clock_hands);


  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  layer_add_child(window_layer, main_clock); 
  layer_add_child(window_layer, clock_hands);

  tick_handler(tick_time, MINUTE_UNIT);
}


static void main_window_unload() {
  layer_destroy(main_clock);
  gbitmap_destroy(H0);
}

static void app_connection_handler(bool connected) {
  if (! connected) {
      static const uint32_t const segments[] = { 1000, 100, 900, 100, 250 };
      VibePattern pat = {
        .durations = segments,
        .num_segments = ARRAY_LENGTH(segments),
      };
      vibes_enqueue_custom_pattern(pat);
  }
}

static void app_focus_changing(bool focusing) {
    if (focusing) {
      Layer *window_layer = window_get_root_layer(my_window);
      layer_set_hidden(window_layer, true);
    }
}

static void app_focus_changed(bool focused) {
    if (focused) {
      Layer *window_layer = window_get_root_layer(my_window);
      layer_set_hidden(window_layer, false);  
    }
}

void handle_init(void) {
  my_window = window_create();
  s_attributes = graphics_text_attributes_create();
  window_set_window_handlers(my_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = app_connection_handler,
  });
  
  // Focus
  app_focus_service_subscribe_handlers((AppFocusHandlers){
    .did_focus = app_focus_changed,
    .will_focus = app_focus_changing
  });
  
  
  window_stack_push(my_window, true);
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

void handle_deinit(void) {
  window_destroy(my_window);
  app_message_deregister_callbacks();
  connection_service_unsubscribe();
  tick_timer_service_unsubscribe();
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}