#include <gtk/gtk.h>
#include <stdlib.h>

GtkWidget *entry1, *entry2, *result_label;

void calculate(GtkWidget *widget, gpointer data) {
    const char *operation = (const char *)data;

    const char *text1 = gtk_entry_get_text(GTK_ENTRY(entry1));
    const char *text2 = gtk_entry_get_text(GTK_ENTRY(entry2));

    double num1 = atof(text1);
    double num2 = atof(text2);
    double result = 0;

    if (strcmp(operation, "+") == 0) {
        result = num1 + num2;
    } else if (strcmp(operation, "-") == 0) {
        result = num1 - num2;
    } else if (strcmp(operation, "*") == 0) {
        result = num1 * num2;
    } else if (strcmp(operation, "/") == 0) {
        if (num2 != 0) {
            result = num1 / num2;
        } else {
            gtk_label_set_text(GTK_LABEL(result_label), "Error: Division by zero");
            return;
        }
    }

    char result_text[50];
    snprintf(result_text, sizeof(result_text), "Result: %.2f", result);
    gtk_label_set_text(GTK_LABEL(result_label), result_text);
}

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *add_button, *sub_button, *mul_button, *div_button;

    gtk_init(&argc, &argv);

    // Create a new window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Simple Calculator");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Create a grid layout
    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    // Create entry widgets
    entry1 = gtk_entry_new();
    entry2 = gtk_entry_new();

    gtk_grid_attach(GTK_GRID(grid), entry1, 0, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), entry2, 0, 1, 2, 1);

    // Create result label
    result_label = gtk_label_new("Result: ");
    gtk_grid_attach(GTK_GRID(grid), result_label, 0, 2, 2, 1);

    // Create buttons
    add_button = gtk_button_new_with_label("+");
    sub_button = gtk_button_new_with_label("-");
    mul_button = gtk_button_new_with_label("*");
    div_button = gtk_button_new_with_label("/");

    gtk_grid_attach(GTK_GRID(grid), add_button, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), sub_button, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), mul_button, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), div_button, 1, 4, 1, 1);

    // Connect signals for buttons
    g_signal_connect(add_button, "clicked", G_CALLBACK(calculate), "+");
    g_signal_connect(sub_button, "clicked", G_CALLBACK(calculate), "-");
    g_signal_connect(mul_button, "clicked", G_CALLBACK(calculate), "*");
    g_signal_connect(div_button, "clicked", G_CALLBACK(calculate), "/");

    // Show all widgets
    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
