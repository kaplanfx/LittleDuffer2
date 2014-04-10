#include <pebble.h>

//define some frame sizes
#define HOLE_HEADER_FRAME (GRect(0,80,70,30))
#define TOTAL_HEADER_FRAME (GRect(74,80,70,30))
#define HOLE_COUNT_FRAME (GRect(0,142-32,70,48))
#define TOTAL_COUNT_FRAME (GRect(74,142-32,70,48))
	
//define keys for persistent storage
#define CURRENT_COUNT_KEY 0
#define CURRENT_HOLE_KEY 1
#define TOTAL_COUNT_KEY 2

//define my main window	
static Window *window;

//define my text layers
TextLayer *hole_header_layer; /* header for hole count */
TextLayer *hole_count_layer; /* tracks which hole you are on */
TextLayer *total_header_layer; /* header for total count */
TextLayer *total_count_layer; /* tracks your total score for the round */

//custom font vars
GFont duepuntozero_bold_24; /* my custom font */
GFont duepuntozero_bold_38; /* large size */

//this is the main var that hold the count
int current_count = 0;
//this var holds the current hole
int current_hole = 0;
//this var holds the total count
int total_count = 0;

//define char strings for my headers
static char hole_header_text[] = "HOLE:";
static char total_header_text[] = "TOT:";

//define char buffs for hole and total count
static char hole_text[3] = "000";
static char total_text[4] = "0000";

// All the image setup stuff is here
// Image resources
#define NUMBER_OF_TIME_IMAGES 10
const int IMAGE_RESOURCE_IDS[NUMBER_OF_TIME_IMAGES] = {
  RESOURCE_ID_IMAGE_TIME_0, 
  RESOURCE_ID_IMAGE_TIME_1, RESOURCE_ID_IMAGE_TIME_2, RESOURCE_ID_IMAGE_TIME_3, 
  RESOURCE_ID_IMAGE_TIME_4, RESOURCE_ID_IMAGE_TIME_5, RESOURCE_ID_IMAGE_TIME_6, 
  RESOURCE_ID_IMAGE_TIME_7, RESOURCE_ID_IMAGE_TIME_8, RESOURCE_ID_IMAGE_TIME_9
};

// Set up the image slots, there are 3 total, either a single centered in slot 0
// or two digit numbers in slots 1 and 2
#define TOTAL_IMAGE_SLOTS 3

// create an array of BmpContainers to store the image resources
static GBitmap *images[TOTAL_IMAGE_SLOTS];
static BitmapLayer *image_layers[TOTAL_IMAGE_SLOTS];

// Empty slots are set to negative 1, the will be unloaded and not drawn
#define EMPTY_SLOT -1

// array to store the current state of a slot
int image_slot_state[TOTAL_IMAGE_SLOTS] = {EMPTY_SLOT, EMPTY_SLOT, EMPTY_SLOT};

//load appropriate images onto screen
void load_digit_image_into_slot(int slot_number, int digit_value) {
  /*

     Loads the digit image from the application's resources and
     displays it on-screen in the correct location.

     Each slot is a quarter of the screen.

   */

  // TODO: Signal these error(s)?

  if ((slot_number < 0) || (slot_number >= TOTAL_IMAGE_SLOTS)) {
    return;
  }

  if ((digit_value < 0) || (digit_value > 9)) {
    return;
  }

  if (image_slot_state[slot_number] != EMPTY_SLOT) {
    return;
  }

  // Set the slot state of the set slot number equal to the digit to be displayed
  image_slot_state[slot_number] = digit_value;
  images[slot_number] = gbitmap_create_with_resource(IMAGE_RESOURCE_IDS[digit_value]); 

  GRect frame;
	
  // Logic to display the image at the correct location based on the slot_number
  if (slot_number == 0) {
	frame = (GRect) {
      .origin = { 37, 9 },
      .size = images[slot_number]->bounds.size
    };
	
  }
	
  if (slot_number == 1) {
	frame = (GRect) {
      .origin = { 0, 9 },
      .size = images[slot_number]->bounds.size
    };
  }
	
  if (slot_number == 2) {
	frame = (GRect) {
      .origin = { 74, 9 },
      .size = images[slot_number]->bounds.size
    };
  }
  
  // Now add to the window layer
  BitmapLayer *bitmap_layer = bitmap_layer_create(frame);
  image_layers[slot_number] = bitmap_layer;
  bitmap_layer_set_bitmap(bitmap_layer, images[slot_number]);
  Layer *window_layer = window_get_root_layer(window);
  layer_add_child(window_layer, bitmap_layer_get_layer(bitmap_layer));

}

//code to unload a digit image from display
void unload_digit_image_from_slot(int slot_number) {
  /*

     Removes the digit from the display and unloads the image resource
     to free up RAM.

     Can handle being called on an already empty slot.

   */
	
  if (image_slot_state[slot_number] != EMPTY_SLOT) {
    layer_remove_from_parent(bitmap_layer_get_layer(image_layers[slot_number]));
    bitmap_layer_destroy(image_layers[slot_number]);
    gbitmap_destroy(images[slot_number]);
    image_slot_state[slot_number] = EMPTY_SLOT;
  }

}

//Code takes current_count and calls the appropriate load / unload image routines
//most of the custom logic is here
void update_count() {

  int ones = 0, tens = 0;

  //first unload any resources, the resource function checks if they are loaded before attempting
    for (int i = 0; i < TOTAL_IMAGE_SLOTS; i++) {
       unload_digit_image_from_slot(i);
    }

  // If the count is less than 10 load slot 0 with the_count
  if(current_count < 10) {

    load_digit_image_into_slot(0, current_count);
  }

  // If the count is 10 or greater, figure out the ones and tens and draw accordingly
  if(current_count > 9) {

    //a bit of math to get the ones and tens digits
    ones = current_count % 10;
    tens = current_count / 10;
    //load tens in slot 1
    load_digit_image_into_slot(1, tens);
    //load ones into slot 2
    load_digit_image_into_slot(2, ones);
  }

}

//draw the hole count on screen
void update_hole_count() 
{
  //update the char buffer with the correct hole count
  snprintf(hole_text, 3, "%d", current_hole);

  //draw it onto our layer
  text_layer_set_text(hole_count_layer, hole_text);
}

//draw total count on screen
void update_total_count()
{
  //update the char buffer with the current total
  snprintf(total_text, 4, "%d", total_count);

  //draw it onto our layer
  text_layer_set_text(total_count_layer, total_text);
}

//function that calls all the various display updates
void update_display()
{
  update_count();
  update_hole_count();
  update_total_count();
}

//Button handler for single up button press - decrements current count and calls draw
//If current count is zero, count will stay zero
void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {

  if (current_count == 0) {
    current_count = 0;
  } else {
    current_count -= 1;
  }

  update_display();
}

//Button handler for single down button press - increments current count and calls draw
//Rolls over back to 0 at 99 automatically
void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {

  if (current_count == 99) {
    current_count = 0;
  } else {
    current_count += 1;
  }

  update_display();
}

/* Button handler for single select button press
   adds current count to total
   increments hole count
   resets current count */

static void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
	
  if (current_hole == 18) {
    //add current count to total
    total_count += current_count;

    //reset current count to 0
    current_count = 0;

    // don't increment hole
    // need to do something more sophisticated here since people can still add strokes
  } else {
    //first add curent count to total count
    total_count += current_count;

    //increment the hole count
    ++current_hole;

    //reset the current count to 0
    current_count = 0;
  }

  //redraw the display
  update_display();
}

//long click resets all counters
static void select_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
  current_count = 0;
  current_hole = 1;
  total_count = 0;
  
  update_display();
}

//handle button clicks here
static void click_config_provider(Window *window) {

  //single click on select calls select_single_click_handler()
  window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) select_single_click_handler);
	
  //
  window_long_click_subscribe(BUTTON_ID_SELECT, 1000, (ClickHandler) select_long_click_handler, NULL);
	
  //single click on up calls up_single_click_handler;
  window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) up_single_click_handler);
	
  //single click on down call down_single_click_handler
  window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) down_single_click_handler);
}

//handle the main initialization
void handle_init(void) {
	
  //initialize the main window and put it on the window stack
  window = window_create();
  window_stack_push(window, true /* Animated */);
	
  //Get window root layer
  Layer *window_layer = window_get_root_layer(window);

  //set the background to black
  window_set_background_color(window, GColorBlack);

  //load font resources now that resources are initiziled
  duepuntozero_bold_24 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DUEPUNTOZERO_BOLD_24));
  duepuntozero_bold_38 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DUEPUNTOZERO_BOLD_38));

  //draw our hole header
  hole_header_layer = text_layer_create(HOLE_HEADER_FRAME);
  text_layer_set_text_color(hole_header_layer, GColorWhite);
  text_layer_set_background_color(hole_header_layer, GColorClear);
  text_layer_set_font(hole_header_layer, duepuntozero_bold_24);
  text_layer_set_text_alignment(hole_header_layer, GTextAlignmentCenter);
  text_layer_set_text(hole_header_layer, hole_header_text);
  layer_add_child(window_layer, text_layer_get_layer(hole_header_layer));
  
  //draw our total header
  total_header_layer = text_layer_create(TOTAL_HEADER_FRAME);
  text_layer_set_text_color(total_header_layer, GColorWhite);
  text_layer_set_background_color(total_header_layer, GColorClear);
  text_layer_set_font(total_header_layer, duepuntozero_bold_24);
  text_layer_set_text_alignment(total_header_layer, GTextAlignmentCenter);
  text_layer_set_text(total_header_layer, total_header_text);
  layer_add_child(window_layer, text_layer_get_layer(total_header_layer));

  //setup our hole text layer
  hole_count_layer = text_layer_create(HOLE_COUNT_FRAME);
  text_layer_set_text_color(hole_count_layer, GColorBlack);
  text_layer_set_background_color(hole_count_layer, GColorWhite);
  text_layer_set_font(hole_count_layer, duepuntozero_bold_38);
  text_layer_set_text_alignment(hole_count_layer, GTextAlignmentCenter);
  layer_set_frame(text_layer_get_layer(hole_count_layer), GRect(0,142-36,70,48));
  layer_add_child(window_layer, text_layer_get_layer(hole_count_layer));
  //don't add the text here

  //setup our total text layer
  total_count_layer = text_layer_create(TOTAL_COUNT_FRAME);
  text_layer_set_text_color(total_count_layer, GColorBlack);
  text_layer_set_background_color(total_count_layer, GColorWhite);
  text_layer_set_font(total_count_layer, duepuntozero_bold_38);
  text_layer_set_text_alignment(total_count_layer, GTextAlignmentCenter);
  layer_set_frame(text_layer_get_layer(total_count_layer), GRect(74,142-36,70,48));
  layer_add_child(window_layer, text_layer_get_layer(total_count_layer));

  //button handler init
  window_set_click_config_provider(window, (ClickConfigProvider) click_config_provider);

  //check for persistant storage else load default values
  //make sure the count set to persistent or 0
  current_count = persist_exists(CURRENT_COUNT_KEY) ? persist_read_int(CURRENT_COUNT_KEY) : 0;
  //make sure the hole starts at 1
  current_hole = persist_exists(CURRENT_HOLE_KEY) ? persist_read_int(CURRENT_HOLE_KEY) : 1;
  //make sure the total is 0
  total_count = persist_exists(TOTAL_COUNT_KEY) ? persist_read_int(TOTAL_COUNT_KEY) : 0;

  //update the display
  update_display();
}

void handle_deinit(void) {
  	
  // Iterate through the image slots and make sure they are unloaded
  for (int i = 0; i < TOTAL_IMAGE_SLOTS; i++) {
    unload_digit_image_from_slot(i);
  }
	
  //unload my layers
  text_layer_destroy(hole_header_layer);
  text_layer_destroy(total_header_layer);
  text_layer_destroy(hole_count_layer);
  text_layer_destroy(total_count_layer);
	
  //unload my custom fonts
  fonts_unload_custom_font(duepuntozero_bold_24);
  fonts_unload_custom_font(duepuntozero_bold_38);
	
  //save the current state of counters
  persist_write_int(CURRENT_COUNT_KEY, current_count);
  persist_write_int(CURRENT_HOLE_KEY, current_hole);
  persist_write_int(TOTAL_COUNT_KEY, total_count);
	
  //destroy the main window	
  window_destroy(window);
}

int main(void) {
	  handle_init();
	  app_event_loop();
	  handle_deinit();
}
