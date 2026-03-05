/*
 * Pane — Stage 6 Real-World Site Test Harness
 *
 * Reads real-world-style HTML test files from tests/realworld/,
 * renders them through the Pane engine, validates layout geometry,
 * and reports statistics including failure classification.
 */

#include "../src/pane.h"
#include "../src/font/font.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

/* ── Configuration ─────────────────────────────────────────────────── */

#define TOLERANCE  2.0f   /* Accept 2px rounding differences for real-world pages */
#define VIEWPORT_W 800
#define VIEWPORT_H 600
#define MAX_TESTS  64
#define MAX_CHECKS 32
#define MAX_PATH   1024

/* ── Expected geometry for one element ─────────────────────────────── */

typedef struct {
    char id[64];
    float x, y, width, height;
    bool has_x, has_y, has_width, has_height;
} ExpectedBox;

/* ── Test case ─────────────────────────────────────────────────────── */

typedef struct {
    char name[128];
    char file[256];
    char description[256];
    ExpectedBox checks[MAX_CHECKS];
    int check_count;
} TestSpec;

/* ── Failure classification ────────────────────────────────────────── */

typedef enum {
    FAIL_NONE,
    FAIL_ELEMENT_NOT_FOUND,
    FAIL_POSITION_X,
    FAIL_POSITION_Y,
    FAIL_SIZE_WIDTH,
    FAIL_SIZE_HEIGHT,
} FailType;

static const char *fail_type_name(FailType ft) {
    switch (ft) {
    case FAIL_ELEMENT_NOT_FOUND: return "element-not-found";
    case FAIL_POSITION_X: return "position-x";
    case FAIL_POSITION_Y: return "position-y";
    case FAIL_SIZE_WIDTH: return "size-width";
    case FAIL_SIZE_HEIGHT: return "size-height";
    default: return "none";
    }
}

/* ── Minimal JSON parsing ──────────────────────────────────────────── */

static const char *skip_ws(const char *p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    return p;
}

static const char *parse_string(const char *p, char *out, size_t max) {
    p = skip_ws(p);
    if (*p != '"') return NULL;
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i < max - 1) {
        if (*p == '\\') { p++; if (!*p) return NULL; }
        out[i++] = *p++;
    }
    out[i] = '\0';
    if (*p == '"') p++;
    return p;
}

static const char *parse_number(const char *p, float *out) {
    p = skip_ws(p);
    char buf[32];
    int i = 0;
    if (*p == '-') buf[i++] = *p++;
    while ((*p >= '0' && *p <= '9') || *p == '.') {
        if (i < 30) buf[i++] = *p;
        p++;
    }
    buf[i] = '\0';
    *out = (float)atof(buf);
    return p;
}

static const char *expect_char(const char *p, char c) {
    p = skip_ws(p);
    if (*p == c) return p + 1;
    return NULL;
}

static const char *parse_checks(const char *p, TestSpec *spec) {
    p = expect_char(p, '{');
    if (!p) return NULL;
    p = skip_ws(p);

    while (*p && *p != '}') {
        ExpectedBox *box = &spec->checks[spec->check_count];
        memset(box, 0, sizeof(*box));

        p = parse_string(p, box->id, sizeof(box->id));
        if (!p) return NULL;
        p = expect_char(p, ':');
        if (!p) return NULL;

        p = expect_char(p, '{');
        if (!p) return NULL;
        p = skip_ws(p);

        while (*p && *p != '}') {
            char key[32];
            p = parse_string(p, key, sizeof(key));
            if (!p) return NULL;
            p = expect_char(p, ':');
            if (!p) return NULL;
            float val;
            p = parse_number(p, &val);
            if (!p) return NULL;

            if (strcmp(key, "x") == 0) { box->x = val; box->has_x = true; }
            else if (strcmp(key, "y") == 0) { box->y = val; box->has_y = true; }
            else if (strcmp(key, "width") == 0) { box->width = val; box->has_width = true; }
            else if (strcmp(key, "height") == 0) { box->height = val; box->has_height = true; }

            p = skip_ws(p);
            if (*p == ',') p++;
            p = skip_ws(p);
        }
        p = expect_char(p, '}');
        if (!p) return NULL;
        spec->check_count++;

        p = skip_ws(p);
        if (*p == ',') p++;
        p = skip_ws(p);
    }
    p = expect_char(p, '}');
    return p;
}

static int parse_manifest(const char *json, TestSpec *specs, int max_specs) {
    int count = 0;
    const char *p = json;

    p = strstr(p, "\"tests\"");
    if (!p) return 0;
    p += 7;
    p = expect_char(p, ':');
    if (!p) return 0;
    p = expect_char(p, '[');
    if (!p) return 0;
    p = skip_ws(p);

    while (*p && *p != ']' && count < max_specs) {
        TestSpec *spec = &specs[count];
        memset(spec, 0, sizeof(*spec));

        p = expect_char(p, '{');
        if (!p) break;
        p = skip_ws(p);

        while (*p && *p != '}') {
            char key[64];
            p = parse_string(p, key, sizeof(key));
            if (!p) break;
            p = expect_char(p, ':');
            if (!p) break;

            if (strcmp(key, "name") == 0) {
                p = parse_string(p, spec->name, sizeof(spec->name));
            } else if (strcmp(key, "file") == 0) {
                p = parse_string(p, spec->file, sizeof(spec->file));
            } else if (strcmp(key, "description") == 0) {
                p = parse_string(p, spec->description, sizeof(spec->description));
            } else if (strcmp(key, "checks") == 0) {
                p = parse_checks(p, spec);
            } else {
                /* Skip unknown value */
                p = skip_ws(p);
                if (*p == '"') {
                    char tmp[512];
                    p = parse_string(p, tmp, sizeof(tmp));
                } else if (*p == '{') {
                    int depth = 1; p++;
                    while (*p && depth > 0) {
                        if (*p == '{') depth++;
                        else if (*p == '}') depth--;
                        p++;
                    }
                } else if (*p == '[') {
                    int depth = 1; p++;
                    while (*p && depth > 0) {
                        if (*p == '[') depth++;
                        else if (*p == ']') depth--;
                        p++;
                    }
                } else {
                    while (*p && *p != ',' && *p != '}') p++;
                }
            }
            if (!p) break;
            p = skip_ws(p);
            if (*p == ',') p++;
            p = skip_ws(p);
        }
        p = expect_char(p, '}');
        if (!p) break;
        count++;

        p = skip_ws(p);
        if (*p == ',') p++;
        p = skip_ws(p);
    }
    return count;
}

/* ── Read file into buffer ─────────────────────────────────────────── */

static char *read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    if (out_len) *out_len = len;
    return buf;
}

/* ── Find layout box by DOM element ID ─────────────────────────────── */

static LayoutBox *find_box_by_id(LayoutBox *box, const char *id) {
    if (!box) return NULL;
    if (box->node && box->node->type == PANE_NODE_ELEMENT && box->node->elem.id) {
        if (strcmp(box->node->elem.id, id) == 0) return box;
    }
    for (LayoutBox *child = box->first_child; child; child = child->next_sibling) {
        LayoutBox *found = find_box_by_id(child, id);
        if (found) return found;
    }
    return NULL;
}

/* ── Check one geometry value ──────────────────────────────────────── */

static bool check_value(const char *test_name, const char *elem_id,
                        const char *prop, float expected, float actual,
                        int *fail_count, FailType *fail_type) {
    float diff = fabsf(expected - actual);
    if (diff > TOLERANCE) {
        printf("  FAIL: %s#%s.%s: expected %.1f, got %.1f (diff=%.1f)\n",
               test_name, elem_id, prop, expected, actual, diff);
        (*fail_count)++;
        if (strcmp(prop, "x") == 0) *fail_type = FAIL_POSITION_X;
        else if (strcmp(prop, "y") == 0) *fail_type = FAIL_POSITION_Y;
        else if (strcmp(prop, "width") == 0) *fail_type = FAIL_SIZE_WIDTH;
        else if (strcmp(prop, "height") == 0) *fail_type = FAIL_SIZE_HEIGHT;
        return false;
    }
    return true;
}

/* ── Walk tree and dump layout for debugging ───────────────────────── */

static void dump_box(LayoutBox *box, int depth) {
    for (int i = 0; i < depth; i++) printf("  ");
    const char *id = (box->node && box->node->type == PANE_NODE_ELEMENT && box->node->elem.id)
                     ? box->node->elem.id : "-";
    const char *tag = (box->node && box->node->type == PANE_NODE_ELEMENT)
                      ? box->node->elem.tag_name : "text";
    printf("<%s id=%s> rect=(%.1f,%.1f,%.1f,%.1f)\n",
           tag, id, box->rect.x, box->rect.y, box->rect.width, box->rect.height);
    for (LayoutBox *child = box->first_child; child; child = child->next_sibling)
        dump_box(child, depth + 1);
}

/* ── Main ──────────────────────────────────────────────────────────── */

int main(int argc, char **argv) {
    const char *test_dir = "tests/realworld";
    bool verbose = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0)
            verbose = true;
        else
            test_dir = argv[i];
    }

    printf("Pane Real-World Site Test Harness\n");
    printf("==================================\n\n");

    if (!font_system_init()) {
        fprintf(stderr, "Warning: font system init failed\n");
    }

    char manifest_path[MAX_PATH];
    snprintf(manifest_path, sizeof(manifest_path), "%s/manifest.json", test_dir);
    size_t manifest_len;
    char *manifest_json = read_file(manifest_path, &manifest_len);
    if (!manifest_json) {
        fprintf(stderr, "Error: cannot read %s\n", manifest_path);
        return 1;
    }

    TestSpec specs[MAX_TESTS];
    int test_count = parse_manifest(manifest_json, specs, MAX_TESTS);
    free(manifest_json);

    printf("Loaded %d real-world test specifications\n\n", test_count);

    /* Failure classification counters */
    int fail_by_type[6] = {0};
    int total_pass = 0, total_fail = 0, total_skip = 0;
    int tests_passed = 0, tests_failed = 0;

    for (int i = 0; i < test_count; i++) {
        TestSpec *spec = &specs[i];

        char html_path[MAX_PATH];
        snprintf(html_path, sizeof(html_path), "%s/%s", test_dir, spec->file);
        size_t html_len;
        char *html = read_file(html_path, &html_len);
        if (!html) {
            printf("[SKIP] %s: cannot read %s\n", spec->name, html_path);
            total_skip++;
            continue;
        }

        PaneResult result = pane_render(html, html_len, NULL, 0,
                                         VIEWPORT_W, VIEWPORT_H);
        free(html);

        if (!result.layout_tree || !result.layout_tree->root) {
            printf("[SKIP] %s: no layout tree\n", spec->name);
            pane_result_free(&result);
            total_skip++;
            continue;
        }

        if (verbose) {
            printf("--- %s layout tree ---\n", spec->name);
            dump_box(result.layout_tree->root, 0);
            printf("---\n");
        }

        int fail_count = 0;
        int check_count = 0;

        for (int j = 0; j < spec->check_count; j++) {
            ExpectedBox *exp = &spec->checks[j];
            LayoutBox *box = find_box_by_id(result.layout_tree->root, exp->id);

            if (!box) {
                printf("  FAIL: %s#%s: element not found in layout tree\n",
                       spec->name, exp->id);
                fail_count++;
                fail_by_type[FAIL_ELEMENT_NOT_FOUND]++;
                continue;
            }

            FailType ft = FAIL_NONE;
            if (exp->has_x) {
                check_count++;
                if (!check_value(spec->name, exp->id, "x", exp->x, box->rect.x, &fail_count, &ft))
                    fail_by_type[ft]++;
            }
            if (exp->has_y) {
                check_count++;
                if (!check_value(spec->name, exp->id, "y", exp->y, box->rect.y, &fail_count, &ft))
                    fail_by_type[ft]++;
            }
            if (exp->has_width) {
                check_count++;
                if (!check_value(spec->name, exp->id, "width", exp->width, box->rect.width, &fail_count, &ft))
                    fail_by_type[ft]++;
            }
            if (exp->has_height) {
                check_count++;
                if (!check_value(spec->name, exp->id, "height", exp->height, box->rect.height, &fail_count, &ft))
                    fail_by_type[ft]++;
            }
        }

        if (fail_count == 0) {
            printf("[PASS] %s (%d checks) — %s\n", spec->name, check_count, spec->description);
            total_pass += check_count;
            tests_passed++;
        } else {
            printf("[FAIL] %s (%d/%d checks failed) — %s\n",
                   spec->name, fail_count, check_count + fail_count, spec->description);
            total_fail += fail_count;
            total_pass += (check_count - fail_count);
            tests_failed++;
        }

        pane_result_free(&result);
    }

    /* Summary */
    printf("\n==================================\n");
    printf("Results: %d tests (%d passed, %d failed, %d skipped)\n",
           test_count, tests_passed, tests_failed, total_skip);
    printf("Checks:  %d passed, %d failed\n", total_pass, total_fail);
    printf("==================================\n");

    /* Failure classification */
    if (total_fail > 0) {
        printf("\nFailure Classification:\n");
        for (int i = 1; i <= 5; i++) {
            if (fail_by_type[i] > 0) {
                printf("  %-20s: %d failures\n", fail_type_name((FailType)i), fail_by_type[i]);
            }
        }
    }

    font_system_shutdown();
    return tests_failed > 0 ? 1 : 0;
}
