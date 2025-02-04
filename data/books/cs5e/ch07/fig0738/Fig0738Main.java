package fig0738;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

/**
 * Figure 7.38 of J Stanley Warford, <i>Computer Systems</i>, Fifth edition,
 * Jones & Bartlett, 2017.
 *
 * <p>
 * A translator with a lexical analyzer, a parser, and a code generator.
 *
 * <p>
 * File: <code>Fig0738Main.java</code>
 *
 * @see <a href="http://www.jblearning.com/catalog/9780763771447/"><i>Computer
 * Systems</i></a> publisher home page, <a
 * href="http://www.cslab.pepperdine.edu/warford/cosc330/">course</a> home page.
 * @author J. Stanley Warford
 */
public class Fig0738Main implements ActionListener {

   final JFrame mainWindowFrame;
   final JPanel inputPanel;
   final JTextArea textArea;
   final JPanel buttonPanel;
   final JButton button;

   public Fig0738Main() {
      // Set up the main window.
      mainWindowFrame = new JFrame("Figure 7.38");
      mainWindowFrame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
      mainWindowFrame.setSize(new Dimension(240, 120));

      // Lay out the text area input panel from top to bottom.
      inputPanel = new JPanel();
      inputPanel.setLayout(new BoxLayout(inputPanel, BoxLayout.PAGE_AXIS));
      textArea = new JTextArea();
      JScrollPane scrollPane = new JScrollPane(textArea);
      scrollPane.setPreferredSize(new Dimension(250, 250));
      inputPanel.add(scrollPane);
      inputPanel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));

      // Lay out the button from left to right.
      buttonPanel = new JPanel();
      buttonPanel.setLayout(new BoxLayout(buttonPanel, BoxLayout.LINE_AXIS));
      buttonPanel.setBorder(BorderFactory.createEmptyBorder(0, 10, 10, 10));
      buttonPanel.add(Box.createHorizontalGlue());
      button = new JButton("Translate");
      buttonPanel.add(button);
      buttonPanel.add(Box.createRigidArea(new Dimension(10, 0)));

      // Combine the input panel and the button panel in the main window.
      mainWindowFrame.add(inputPanel, BorderLayout.CENTER);
      mainWindowFrame.add(buttonPanel, BorderLayout.PAGE_END);

      button.addActionListener(this);

      mainWindowFrame.pack();
      mainWindowFrame.setVisible(true);
   }

   private static void createAndShowGUI() {
      JFrame.setDefaultLookAndFeelDecorated(true);
      new Fig0738Main();
   }

   public static void main(String[] args) {
      javax.swing.SwingUtilities.invokeLater(Fig0738Main::createAndShowGUI);
   }

   @Override
   public void actionPerformed(ActionEvent event) {
      InBuffer inBuffer = new InBuffer(textArea.getText());
      Translator tr = new Translator(inBuffer);
      tr.translate();
   }
}
