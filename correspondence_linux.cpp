static void on_text_changed(GtkTextBuffer *buffer, gpointer user_data) {
        // Save state for certain operations
        // This is a simplified version - you might want to be more selective
    }#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string>
#include <stack>
#include <fstream>
#include <iostream>

class CorrespondenceApp {
private:
    GtkWidget *window;
    GtkWidget *text_view;
    GtkTextBuffer *text_buffer;
    GtkWidget *status_bar;
    guint status_context_id;
    
    bool enable_substitution = false;
    std::stack<std::string> undo_stack;
    std::stack<std::string> redo_stack;
    
    // Character mapping for encoding/decoding
    const std::pair<char, const char*> char_map[26] = {
        {'a', "☉"}, {'b', "●"}, {'c', "☾"}, {'d', "☽"}, {'e', "○"},
        {'f', "☿"}, {'g', "♀"}, {'h', "♁"}, {'i', "♂"}, {'j', "♃"},
        {'k', "♄"}, {'l', "♅"}, {'m', "♆"}, {'n', "♇"}, {'o', "♈"},
        {'p', "♉"}, {'q', "♊"}, {'r', "♋"}, {'s', "♌"}, {'t', "♍"},
        {'u', "♎"}, {'v', "♏"}, {'w', "♐"}, {'x', "♑"}, {'y', "♒"}, {'z', "♓"}
    };

public:
    CorrespondenceApp() = default;
    
    void save_current_state_for_undo() {
        GtkTextIter start, end;
        gtk_text_buffer_get_bounds(text_buffer, &start, &end);
        gchar *text = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
        
        undo_stack.push(std::string(text));
        while (!redo_stack.empty()) {
            redo_stack.pop();
        }
        
        g_free(text);
    }
    
    void update_status_bar() {
        gtk_statusbar_pop(GTK_STATUSBAR(status_bar), status_context_id);
        std::string status = "Code " + std::string(enable_substitution ? "Enabled" : "Disabled");
        gtk_statusbar_push(GTK_STATUSBAR(status_bar), status_context_id, status.c_str());
    }
    
    void encode_text() {
        save_current_state_for_undo();
        
        GtkTextIter start, end;
        gtk_text_buffer_get_bounds(text_buffer, &start, &end);
        gchar *text = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
        
        std::string encoded_text;
        for (char *p = text; *p; ++p) {
            char c = tolower(*p);
            bool found = false;
            
            for (int i = 0; i < 26; ++i) {
                if (c == char_map[i].first) {
                    encoded_text += char_map[i].second;
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                encoded_text += *p;
            }
        }
        
        gtk_text_buffer_set_text(text_buffer, encoded_text.c_str(), -1);
        g_free(text);
        update_status_bar();
    }
    
    void decode_text() {
        save_current_state_for_undo();
        
        GtkTextIter start, end;
        gtk_text_buffer_get_bounds(text_buffer, &start, &end);
        gchar *text = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
        
        std::string decoded_text;
        const char *p = text;
        
        while (*p) {
            bool found = false;
            
            for (int i = 0; i < 26; ++i) {
                const char *symbol = char_map[i].second;
                size_t symbol_len = strlen(symbol);
                
                if (strncmp(p, symbol, symbol_len) == 0) {
                    decoded_text += char_map[i].first;
                    p += symbol_len;
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                decoded_text += *p;
                ++p;
            }
        }
        
        gtk_text_buffer_set_text(text_buffer, decoded_text.c_str(), -1);
        g_free(text);
        update_status_bar();
    }
    
    void undo() {
        if (!undo_stack.empty()) {
            GtkTextIter start, end;
            gtk_text_buffer_get_bounds(text_buffer, &start, &end);
            gchar *current_text = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
            
            std::string previous_state = undo_stack.top();
            redo_stack.push(std::string(current_text));
            undo_stack.pop();
            
            gtk_text_buffer_set_text(text_buffer, previous_state.c_str(), -1);
            g_free(current_text);
        }
    }
    
    void redo() {
        if (!redo_stack.empty()) {
            GtkTextIter start, end;
            gtk_text_buffer_get_bounds(text_buffer, &start, &end);
            gchar *current_text = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
            
            std::string next_state = redo_stack.top();
            undo_stack.push(std::string(current_text));
            redo_stack.pop();
            
            gtk_text_buffer_set_text(text_buffer, next_state.c_str(), -1);
            g_free(current_text);
        }
    }
    
    void save_file() {
        GtkWidget *dialog = gtk_file_chooser_dialog_new("Save File",
            GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_SAVE,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Save", GTK_RESPONSE_ACCEPT, NULL);
        
        gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
        
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            
            GtkTextIter start, end;
            gtk_text_buffer_get_bounds(text_buffer, &start, &end);
            gchar *text = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
            
            std::ofstream file(filename);
            if (file.is_open()) {
                file << text;
                file.close();
                
                GtkWidget *success_dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                    GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                    "File saved successfully!");
                gtk_dialog_run(GTK_DIALOG(success_dialog));
                gtk_widget_destroy(success_dialog);
            }
            
            g_free(filename);
            g_free(text);
        }
        
        gtk_widget_destroy(dialog);
    }
    
    void open_file() {
        GtkWidget *dialog = gtk_file_chooser_dialog_new("Open File",
            GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN,
            "_Cancel", GTK_RESPONSE_CANCEL,
            "_Open", GTK_RESPONSE_ACCEPT, NULL);
        
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            
            std::ifstream file(filename);
            if (file.is_open()) {
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                file.close();
                
                gtk_text_buffer_set_text(text_buffer, content.c_str(), -1);
            }
            
            g_free(filename);
        }
        
        gtk_widget_destroy(dialog);
    }
    
    // Static callback functions for GTK
    static void on_open_activate(GtkWidget *widget, gpointer data) {
        static_cast<CorrespondenceApp*>(data)->open_file();
    }
    
    static void on_save_activate(GtkWidget *widget, gpointer data) {
        static_cast<CorrespondenceApp*>(data)->save_file();
    }
    
    static void on_quit_activate(GtkWidget *widget, gpointer data) {
        gtk_main_quit();
    }
    
    static void on_encode_activate(GtkWidget *widget, gpointer data) {
        static_cast<CorrespondenceApp*>(data)->encode_text();
    }
    
    static void on_decode_activate(GtkWidget *widget, gpointer data) {
        static_cast<CorrespondenceApp*>(data)->decode_text();
    }
    
    static void on_toggle_activate(GtkWidget *widget, gpointer data) {
        CorrespondenceApp* app = static_cast<CorrespondenceApp*>(data);
        app->enable_substitution = !app->enable_substitution;
        app->update_status_bar();
    }
    
    static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
        CorrespondenceApp *app = static_cast<CorrespondenceApp*>(user_data);
        
        if (event->state & GDK_CONTROL_MASK) {
            switch (event->keyval) {
                case GDK_KEY_e:
                case GDK_KEY_E:
                    app->encode_text();
                    return TRUE;
                case GDK_KEY_d:
                case GDK_KEY_D:
                    app->decode_text();
                    return TRUE;
                case GDK_KEY_t:
                case GDK_KEY_T:
                    app->enable_substitution = !app->enable_substitution;
                    app->update_status_bar();
                    return TRUE;
                case GDK_KEY_s:
                case GDK_KEY_S:
                    app->save_file();
                    return TRUE;
                case GDK_KEY_o:
                case GDK_KEY_O:
                    app->open_file();
                    return TRUE;
                case GDK_KEY_q:
                case GDK_KEY_Q:
                    gtk_main_quit();
                    return TRUE;
                case GDK_KEY_z:
                case GDK_KEY_Z:
                    app->undo();
                    return TRUE;
                case GDK_KEY_y:
                case GDK_KEY_Y:
                    app->redo();
                    return TRUE;
            }
        }
        
        // Handle character substitution in real-time
        if (app->enable_substitution && !(event->state & GDK_CONTROL_MASK)) {
            char c = tolower(event->keyval);
            for (int i = 0; i < 26; ++i) {
                if (c == app->char_map[i].first) {
                    GtkTextMark *mark = gtk_text_buffer_get_insert(app->text_buffer);
                    GtkTextIter iter;
                    gtk_text_buffer_get_iter_at_mark(app->text_buffer, &iter, mark);
                    gtk_text_buffer_insert(app->text_buffer, &iter, app->char_map[i].second, -1);
                    return TRUE;
                }
            }
        }
        
        return FALSE;
    }
    
    void create_menu_bar() {
        GtkWidget *menu_bar = gtk_menu_bar_new();
        
        // File menu
        GtkWidget *file_menu = gtk_menu_new();
        GtkWidget *file_item = gtk_menu_item_new_with_label("File");
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
        
        GtkWidget *open_item = gtk_menu_item_new_with_label("Open - Ctrl+O");
        GtkWidget *save_item = gtk_menu_item_new_with_label("Save - Ctrl+S");
        GtkWidget *quit_item = gtk_menu_item_new_with_label("Quit - Ctrl+Q");
        
        gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);
        gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save_item);
        gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), gtk_separator_menu_item_new());
        gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_item);
        
        // Code menu
        GtkWidget *code_menu = gtk_menu_new();
        GtkWidget *code_item = gtk_menu_item_new_with_label("Code");
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(code_item), code_menu);
        
        GtkWidget *encode_item = gtk_menu_item_new_with_label("Encode - Ctrl+E");
        GtkWidget *decode_item = gtk_menu_item_new_with_label("Decode - Ctrl+D");
        GtkWidget *toggle_item = gtk_menu_item_new_with_label("Toggle - Ctrl+T");
        
        gtk_menu_shell_append(GTK_MENU_SHELL(code_menu), encode_item);
        gtk_menu_shell_append(GTK_MENU_SHELL(code_menu), decode_item);
        gtk_menu_shell_append(GTK_MENU_SHELL(code_menu), toggle_item);
        
        gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_item);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), code_item);
        
        // Connect signals
        g_signal_connect(open_item, "activate", G_CALLBACK(on_open_activate), this);
        g_signal_connect(save_item, "activate", G_CALLBACK(on_save_activate), this);
        g_signal_connect(quit_item, "activate", G_CALLBACK(on_quit_activate), nullptr);
        g_signal_connect(encode_item, "activate", G_CALLBACK(on_encode_activate), this);
        g_signal_connect(decode_item, "activate", G_CALLBACK(on_decode_activate), this);
        g_signal_connect(toggle_item, "activate", G_CALLBACK(on_toggle_activate), this);
        
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(window), vbox);
        gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);
        
        // Text view with scrolled window
        GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        
        text_view = gtk_text_view_new();
        text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        
        // Set font - using CSS provider for modern GTK
        GtkCssProvider *css_provider = gtk_css_provider_new();
        const gchar *css_data = "textview { font-family: 'Noto Color Emoji', 'Segoe UI Symbol', 'Apple Color Emoji'; font-size: 16px; }";
        gtk_css_provider_load_from_data(css_provider, css_data, -1, NULL);
        
        GtkStyleContext *context = gtk_widget_get_style_context(text_view);
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(css_provider);
        
        gtk_container_add(GTK_CONTAINER(scrolled), text_view);
        gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
        
        // Status bar
        status_bar = gtk_statusbar_new();
        status_context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(status_bar), "status");
        gtk_box_pack_start(GTK_BOX(vbox), status_bar, FALSE, FALSE, 0);
        
        update_status_bar();
    }
    
    void run() {
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "Correspondence");
        gtk_window_set_default_size(GTK_WINDOW(window), 480, 640);
        
        // Position window on the right side of screen - modern approach
        GdkDisplay *display = gdk_display_get_default();
        GdkMonitor *monitor = gdk_display_get_primary_monitor(display);
        GdkRectangle geometry;
        gdk_monitor_get_geometry(monitor, &geometry);
        
        gint screen_width = geometry.width;
        gint screen_height = geometry.height;
        gtk_window_move(GTK_WINDOW(window), screen_width - 500, (screen_height - 640) / 2);
        
        create_menu_bar();
        
        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
        g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), this);
        
        gtk_widget_show_all(window);
        gtk_main();
    }
};

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    CorrespondenceApp app;
    app.run();
    
    return 0;
}
