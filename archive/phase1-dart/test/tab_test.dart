import 'package:flutter_test/flutter_test.dart';
import 'package:pane/ui/tab.dart';

void main() {
  group('Tab', () {
    test('starts with empty state', () {
      final tab = Tab();
      expect(tab.url, '');
      expect(tab.title, 'New Tab');
      expect(tab.canGoBack, isFalse);
      expect(tab.canGoForward, isFalse);
    });

    test('navigateTo pushes current URL to back history', () {
      final tab = Tab();
      tab.navigateTo('https://a.com');
      expect(tab.url, 'https://a.com');
      expect(tab.canGoBack, isFalse); // First nav has no previous.

      tab.navigateTo('https://b.com');
      expect(tab.url, 'https://b.com');
      expect(tab.canGoBack, isTrue);
    });

    test('navigateTo clears forward history', () {
      final tab = Tab();
      tab.navigateTo('https://a.com');
      tab.navigateTo('https://b.com');
      tab.goBack();
      expect(tab.canGoForward, isTrue);

      tab.navigateTo('https://c.com');
      expect(tab.canGoForward, isFalse);
    });

    test('goBack returns to previous URL', () {
      final tab = Tab();
      tab.navigateTo('https://a.com');
      tab.navigateTo('https://b.com');
      tab.navigateTo('https://c.com');

      final prev = tab.goBack();
      expect(prev, 'https://b.com');
      expect(tab.url, 'https://b.com');
      expect(tab.canGoForward, isTrue);
    });

    test('goForward returns to next URL', () {
      final tab = Tab();
      tab.navigateTo('https://a.com');
      tab.navigateTo('https://b.com');
      tab.goBack();

      final next = tab.goForward();
      expect(next, 'https://b.com');
      expect(tab.url, 'https://b.com');
    });

    test('goBack returns null when no history', () {
      final tab = Tab();
      expect(tab.goBack(), isNull);
    });

    test('goForward returns null when no forward history', () {
      final tab = Tab();
      tab.navigateTo('https://a.com');
      expect(tab.goForward(), isNull);
    });

    test('navigation resets scroll offset', () {
      final tab = Tab();
      tab.scrollOffset = 500;
      tab.navigateTo('https://a.com');
      expect(tab.scrollOffset, 0);
    });

    test('goBack resets scroll offset', () {
      final tab = Tab();
      tab.navigateTo('https://a.com');
      tab.navigateTo('https://b.com');
      tab.scrollOffset = 300;
      tab.goBack();
      expect(tab.scrollOffset, 0);
    });
  });
}
