from ShortFrags.GUI.view import View
from ShortFrags.GUI.text_display import TextDisplay
from ShortFrags.Expt.fragexpt import print_matrix, print_align

class MatrixView(TextDisplay, View):
    def __init__(self, parent=None):
        TextDisplay.__init__(self, parent, label='Matrix')
        View.__init__(self)

    def reset(self, state):
        text = print_matrix(state['matrix'],
                            state['conv_type'],
                            state['scale'],
                            state['weight'],
                            state['reg'],
                            state['FE'],
                            list(state['coords']) + [state['iter']],
                            )
        self.set_text('Matrix', text)
        # if PSSM show distance as well
        if state['matrix'] == 'PSSM':
            text = print_matrix(state['matrix'],
                                "Quasi",
                                state['scale'],
                                state['weight'],
                                state['reg'],
                                state['FE'],
                                list(state['coords']) + [state['iter']],
                                )
            self.insert_text(text)
            # Additional data
            text = print_align(state['weight'],
                               state['reg'],
                               state['FE'],
                               list(state['coords']) + [state['iter']],
                               )
            self.insert_text(text)
        
