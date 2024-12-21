#include <gtk/gtk.h>

// 버튼 클릭 시 실행될 콜백 함수
static void on_button_clicked(GtkWidget *widget, gpointer data) {
    g_print("Button clicked! Message: %s\n", (char *)data);
}
int main(int argc, char *argv[]) {
    // GTK 초기화
    gtk_init(&argc, &argv);
    // 윈도우 생성
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "GTK Example");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
    // 창 닫기 이벤트 처리
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    // 레이아웃 컨테이너 생성
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), box);
    // 레이블 추가
    GtkWidget *label = gtk_label_new("Hello, GTK!");
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
    // 버튼 추가
    GtkWidget *button = gtk_button_new_with_label("Click Me");
    gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
    // 버튼 클릭 이벤트 처리
    g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), "You clicked the button!");
    // 윈도우 표시
    gtk_widget_show_all(window);
    // GTK 메인 루프 실행
    gtk_main();
    return 0;
}